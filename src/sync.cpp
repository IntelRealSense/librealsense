// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "sync.h"
#include <functional>

namespace librealsense
{

	template<class T>
	class internal_frame_processor_callback : public rs2_frame_processor_callback
	{
		T on_frame_function;
	public:
		explicit internal_frame_processor_callback(T on_frame) : on_frame_function(on_frame) {}

		void on_frame(rs2_frame ** f, int count, rs2_source * source) override
		{
			if (count > 0)
			{
				frame_holder front(f[0]);
				on_frame_function(std::move(front), source->source);
			}
		}

		void release() override { delete this; }
	};

    syncer_proccess_unit::syncer_proccess_unit()
        : processing_block(RS2_EXTENSION_TYPE_VIDEO_FRAME, nullptr),
		  _matcher({})
    {
		_matcher.set_callback([this](frame_holder f)
		{
			// TODO: lock
			get_source().frame_ready(std::move(f));
		});

		auto f = [&](frame_holder frame, synthetic_source_interface* source)
		{
			
			// Create single composite matcher M with no children
			// M.set_callback([](f, regratable_lock&){
			//    regratable_lock.early_unlock();
			//    call outside callback(f) // TODO: TRY&CATCH
			// }

			// set_processing_funct( f:
			//		regratable_lock l(m)
			//		M.dispatch(move(f), &regratable_lock)

			// TODO: lock
			_matcher.dispatch(std::move(frame));
		};
		set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
			new internal_frame_processor_callback<decltype(f)>(f)));
	};

	matcher::matcher()
	{}

	void matcher::set_callback(std::function<void(frame_holder)> f)
	{
		_callback = f;
	}

	void  matcher::sync(frame_holder f)
	{
		_callback(std::move(f));
	}

	identity_matcher::identity_matcher(stream_id stream)
	{
		_stream = { stream };
	}

	void identity_matcher::dispatch(frame_holder f) 
	{ 
		sync(std::move(f)); 
	}

	const std::vector<stream_id>& identity_matcher::get_streams() const
	{
		return  _stream;
	}

	composite_matcher::composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
	{
		for (auto&& matcher : matchers)
		{
			for (auto&& stream : matcher->get_streams())
			{
				matcher->set_callback([&](frame_holder f)
				{
					sync(std::move(f));
				});
				_matchers[stream] = matcher;
				_streams.push_back(stream);
			}
		}
	}

	void composite_matcher::dispatch(frame_holder f)
	{
		auto frame_ptr = f.frame->get();
		auto stream = frame_ptr->get_stream_type();

		auto dev = frame_ptr->get_owner()->get_device().lock();

		auto matcher = find_matcher(stream_id(dev.get(), stream));

		matcher->dispatch(std::move(f));
	}

	std::shared_ptr<matcher> composite_matcher::find_matcher(stream_id stream)
	{
		std::shared_ptr<matcher> matcher;

		if(stream.first)
		{
			matcher = _matchers[stream];
			if (!matcher)
			{
				matcher = stream.first->create_matcher(stream.second);

				matcher->set_callback([&](frame_holder f)
				{
					sync(std::move(f));
				});

				for (auto stream : matcher->get_streams())
					_matchers[stream] = matcher;
			}

		}
		else
		{
			matcher = _matchers[stream];
			// We don't know what device this frame came from, so just store it under device NULL with ID matcher
			if (!matcher)
			{
				_matchers[stream] = std::make_shared<identity_matcher>(stream);
				matcher = _matchers[stream];

				matcher->set_callback([&](frame_holder f)
				{
					sync(std::move(f));
				});
			}
		}
		return matcher;
	}

	void composite_matcher::sync(frame_holder f)
	{
		auto frame_ptr = f.frame->get();
		auto stream = frame_ptr->get_stream_type();

		auto matcher = find_matcher(stream_id(frame_ptr->get_owner()->get_device().lock().get(), stream));
		_frames_queue[matcher.get()].enqueue(std::move(f));
		
		std::vector<frame_holder*> frames;
		std::vector<librealsense::matcher*> frames_matcher;
		std::vector<librealsense::matcher*> synced_frames;

		std::vector<librealsense::matcher*> missing_streams;

		std::vector<frame_holder> synced;

		do
		{
			synced_frames.clear();
			frames.clear();


			for (auto s = _frames_queue.begin(); s != _frames_queue.end(); s++)
			{
				frame_holder* f;
				if (s->second.peek(&f))
				{
					frames.push_back(f);
					frames_matcher.push_back(s->first);
				}
				else
				{
					missing_streams.push_back(s->first);
				}
			}
			if(frames.size())
				std::cout << "QUEUES: ";
			for (auto f : frames)
			{
				std::cout << (*f)->get()->get_stream_type() << " " << (*f)->get()->get_frame_number() << " ";
			}
			std::cout << "\n";
			if (frames.size() == 0)
				break;

			frame_holder* curr_sync;
			if (frames.size()> 0)
			{
				curr_sync = frames[0];
				synced_frames.push_back(frames_matcher[0]);
			}
			for (auto i = 1; i < frames.size() ; i++)
			{
				if (are_equivalent(*curr_sync, *frames[i]))
				{
					synced_frames.push_back(frames_matcher[i]);
				}
				else
				{
					if (is_smaller_than(*frames[i] , *curr_sync))
					{
						synced_frames.clear();
						synced_frames.push_back(frames_matcher[i]);
						curr_sync = frames[i];
					}
				}
			}
			
		
			for (auto i : missing_streams)
			{
				if (wait_for_stream(synced_frames, i))
				{
					synced_frames.clear();
					break;
				}
			}
			if(synced_frames.size())
				std::cout << "\nSynced: ";
			for (auto index : synced_frames)
			{
				frame_holder frame;
				_frames_queue[index].dequeue(&frame);

				std::cout << frame->get()->get_stream_type() << " " << frame->get()->get_frame_number() << " " << frame->get()->get_frame_timestamp() << " ";
				//TODO: create composite frame
				synced.push_back(std::move(frame));
			}
				std::cout << "\n";

		} while (synced_frames.size() > 0);

		for (auto&& s : synced)
		{
			_callback(std::move(s));
		}
	}

	const std::vector<stream_id>& composite_matcher::get_streams() const
	{
		return _streams;
	}

	

	frame_number_composite_matcher::frame_number_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
		:composite_matcher(matchers)
	{
	}

	bool frame_number_composite_matcher::are_equivalent(frame_holder& a, frame_holder& b)
	{
		return a->get()->get_frame_number() == b->get()->get_frame_number();
	}
	bool frame_number_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
	{
		return a->get()->get_frame_number() < b->get()->get_frame_number();
	}
	timestamp_composite_matcher::timestamp_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
		:composite_matcher(matchers)
	{
	}
	bool timestamp_composite_matcher::are_equivalent(frame_holder & a, frame_holder & b)
	{
		auto a_fps = a->get()->get_framerate();
		auto b_fps = b->get()->get_framerate();

		auto min_fps = std::min(a_fps, b_fps);

		return  are_equivalent(a->get()->get_frame_timestamp(), b->get()->get_frame_timestamp(), min_fps);
	}

	bool timestamp_composite_matcher::is_smaller_than(frame_holder & a, frame_holder & b)
	{
		return  a->get()->get_frame_timestamp() < b->get()->get_frame_timestamp();
	}

	void timestamp_composite_matcher::dispatch(frame_holder f)
	{
		auto fps = f->get()->get_framerate();

		auto gap = 1000 / fps;

		auto frame_ptr = f.frame->get();
		auto stream = frame_ptr->get_stream_type();


		/*if (auto dev = frame_ptr->get_owner()->get_device().lock())
		{
			_next_expected[std::make_pair(dev.get(), stream)] = f->get()->get_frame_timestamp() + gap;
		}
		else
		{
			_next_expected[std::make_pair(nullptr, stream)] = f->get()->get_frame_timestamp() + gap;
		}*/

		composite_matcher::dispatch(std::move(f));
	}

	bool timestamp_composite_matcher::wait_for_stream(std::vector<matcher*> synced, matcher* missing)
	{
		frame_holder* synced_frame;

		if (_frames_queue[synced[0]].peek(&synced_frame))
		{
			auto next_expected = _next_expected[missing];
			return are_equivalent((*synced_frame)->get()->get_frame_timestamp(), next_expected, (*synced_frame)->get()->get_framerate());
		}
		return true;
	}

	bool timestamp_composite_matcher::are_equivalent(double a, double b, int fps)
	{
		auto gap = 1000 / fps;

		return std::abs(a - b )< gap / 2;
	}
}

