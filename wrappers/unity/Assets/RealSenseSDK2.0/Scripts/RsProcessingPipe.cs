using Intel.RealSense;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

[Serializable]
public class RsProcessingPipe : MonoBehaviour {

    public HashSet<RsProcessingBlock> _processingBlocks = new HashSet<RsProcessingBlock>();

    public void AddProcessingBlock(RsProcessingBlock processingBlock)
    {
        _processingBlocks.Add(processingBlock);
    }
    public void RemoveProcessingBlock(RsProcessingBlock processingBlock)
    {
        _processingBlocks.Remove(processingBlock);
    }

    internal void ProcessFrames(FrameSet frames, FrameSource src, FramesReleaser releaser, Action<Frame> handleFrame, Action<FrameSet> handleFrameSet)
    {
        var pbs = _processingBlocks.OrderBy(i => i.Order).Where(i => i.Enabled).ToList();
        foreach (var vpb in pbs)
        {
            FrameSet processedFrames = frames;
            switch (vpb.ProcessingType)
            {
                case ProcessingBlockType.Single:
                    processedFrames = HandleSingleFrameProcessingBlocks(frames, src, releaser, vpb, handleFrame);
                    break;
                case ProcessingBlockType.Multi:
                    processedFrames = HandleMultiFramesProcessingBlocks(frames, src, releaser, vpb, handleFrameSet);
                    break;
            }
            frames = processedFrames;
        }

        handleFrameSet(frames);
        foreach (var fr in frames)
        {
            using (fr)
                handleFrame(fr);
        }
    }

    private Frame ApplyFilter(Frame frame, FrameSource frameSource, FramesReleaser framesReleaser, RsProcessingBlock vpb, Action<Frame> handleFrame)
    {
        if (!vpb.CanProcess(frame))
            return frame;

        // run the processing block.
        var processedFrame = vpb.Process(frame, frameSource, framesReleaser);

        // incase fork is requested, notify on new frame and use the original frame for the new frameset.
        if (vpb.Fork())
        {
            handleFrame(processedFrame);
            processedFrame.Dispose();
            return frame;
        }

        // avoid disposing the frame incase the filter returns the original frame.
        if (processedFrame == frame)
            return frame;

        // replace the current frame with the processed one to be used as the input to the next iteration (next filter)
        frame.Dispose();
        return processedFrame;
    }

    private FrameSet HandleSingleFrameProcessingBlocks(FrameSet frameSet, FrameSource frameSource, FramesReleaser framesReleaser, RsProcessingBlock videoProcessingBlock, Action<Frame> handleFrame)
    {
        // single frame filters
        List<Frame> processedFrames = new List<Frame>();
        foreach (var frame in frameSet)
        {
            var currFrame = ApplyFilter(frame, frameSource, framesReleaser, videoProcessingBlock, handleFrame);

            // cache the pocessed frame
            processedFrames.Add(currFrame);
            if (frame != currFrame)
                frame.Dispose();
        }

        // Combine the frames into a single frameset
        var newFrameSet = frameSource.AllocateCompositeFrame(framesReleaser, processedFrames.ToArray());

        foreach (var f in processedFrames)
            f.Dispose();

        return newFrameSet;
    }

    private FrameSet HandleMultiFramesProcessingBlocks(FrameSet frameSet, FrameSource frameSource, FramesReleaser framesReleaser, RsProcessingBlock videoProcessingBlock, Action<FrameSet> handleFrameSet)
    {
        using (var frame = frameSet.AsFrame())
        {
            if (videoProcessingBlock.CanProcess(frame))
            {
                using (var f = videoProcessingBlock.Process(frame, frameSource, framesReleaser))
                {
                    if (videoProcessingBlock.Fork())
                    {
                        handleFrameSet(FrameSet.FromFrame(f, framesReleaser));
                    }
                    else
                        return FrameSet.FromFrame(f, framesReleaser);
                }
            }
        }
        return frameSet;
    }
}
