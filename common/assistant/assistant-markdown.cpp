// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include "assistant-markdown.h"
#include "device-model.h"
#include "ux-window.h"
#include "os.h"
#include "../../third-party/imgui_md/imgui_md.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>

namespace rs2
{
    namespace
    {
        // Which frame of an animated image should be showing right now, cycling through
        // frame_delays_ms based on wall-clock time - same idea as the streaming "..." dots.
        size_t current_frame_index(const assistant::cached_image& img, double time_seconds)
        {
            if (img.frame_delays_ms.size() <= 1)
                return 0;

            long long total_ms = 0;
            for (int d : img.frame_delays_ms) total_ms += d;
            if (total_ms <= 0)
                return 0;

            long long elapsed_ms = (long long)(time_seconds * 1000.0) % total_ms;
            long long acc = 0;
            for (size_t i = 0; i < img.frame_delays_ms.size(); i++)
            {
                acc += img.frame_delays_ms[i];
                if (elapsed_ms < acc)
                    return i;
            }
            return img.frame_delays_ms.size() - 1;
        }

        // Deliberately approximate: a small heuristic-only lexer (comments/strings/numbers/keywords/
        // call-identifiers), not a real per-language grammar - good enough for short SDK snippets
        // without pulling in a full syntax-highlighting library for one feature.
        const ImVec4 code_comment_color = from_rgba(0x6a, 0x99, 0x55, 0xff, true);
        const ImVec4 code_string_color = from_rgba(0xce, 0x91, 0x78, 0xff, true);
        const ImVec4 code_keyword_color = from_rgba(0xc5, 0x86, 0xc0, 0xff, true);
        const ImVec4 code_number_color = from_rgba(0xb5, 0xce, 0xa8, 0xff, true);
        const ImVec4 code_call_color = from_rgba(0x7f, 0xc1, 0xe8, 0xff, true);

        const std::unordered_set<std::string>& code_keywords()
        {
            // Merged across Python/C++/JS since a chat snippet's language varies and we don't run
            // a real per-language parser - a few keywords colliding across languages is harmless.
            static const std::unordered_set<std::string> kw = {
                "import", "from", "as", "def", "class", "if", "elif", "else", "for", "while", "return",
                "with", "try", "except", "finally", "pass", "break", "continue", "in", "is", "not",
                "and", "or", "None", "True", "False", "lambda", "yield", "global", "nonlocal", "assert",
                "raise", "del", "include", "using", "namespace", "struct", "public", "private",
                "protected", "static", "const", "void", "int", "float", "double", "bool", "true",
                "false", "nullptr", "new", "delete", "template", "typename", "auto", "switch", "case",
                "default", "function", "let", "var", "extends", "export", "async", "await", "throw",
                "catch", "this", "self",
            };
            return kw;
        }

        struct code_token { std::string text; ImVec4 color; };

        std::vector<code_token> tokenize_code_line(const std::string& line)
        {
            std::vector<code_token> out;
            size_t i = 0, n = line.size();
            while (i < n)
            {
                char c = line[i];
                if (c == '#' || (c == '/' && i + 1 < n && line[i + 1] == '/'))
                {
                    out.push_back({ line.substr(i), code_comment_color });
                    break;
                }
                if (c == '"' || c == '\'')
                {
                    size_t j = i + 1;
                    while (j < n && line[j] != c) j++;
                    if (j < n) j++;
                    out.push_back({ line.substr(i, j - i), code_string_color });
                    i = j;
                    continue;
                }
                if (isdigit((unsigned char)c))
                {
                    size_t j = i;
                    while (j < n && (isdigit((unsigned char)line[j]) || line[j] == '.')) j++;
                    out.push_back({ line.substr(i, j - i), code_number_color });
                    i = j;
                    continue;
                }
                if (isalpha((unsigned char)c) || c == '_')
                {
                    size_t j = i;
                    while (j < n && (isalnum((unsigned char)line[j]) || line[j] == '_')) j++;
                    std::string word = line.substr(i, j - i);
                    bool is_call = (j < n && line[j] == '(');
                    ImVec4 col = code_keywords().count(word) ? code_keyword_color
                        : is_call ? code_call_color : light_grey;
                    out.push_back({ word, col });
                    i = j;
                    continue;
                }
                size_t j = i + 1;
                while (j < n)
                {
                    char cc = line[j];
                    if (isalnum((unsigned char)cc) || cc == '_' || cc == '"' || cc == '\'' || cc == '#') break;
                    if (cc == '/' && j + 1 < n && line[j + 1] == '/') break;
                    j++;
                }
                out.push_back({ line.substr(i, j - i), light_grey });
                i = j;
            }
            return out;
        }

        // A fenced code block's raw text (language tag + body) found by scanning for ```-delimited
        // lines - not a markdown parser, just enough to carve fenced blocks out of `text` so they can
        // be rendered with line numbers/syntax coloring/copy, which imgui_md's private, non-virtual
        // text-rendering pipeline gives no hook to do. Only top-level ``` fences are recognized (the
        // overwhelming common case for LLM output); anything imgui_md itself still encounters - an
        // indented code block, or a fence nested in a list/quote - falls back to BLOCK_CODE below.
        struct code_fence { size_t block_start, block_end; std::string lang, body; };

        std::vector<code_fence> find_fenced_code_blocks(const std::string& text)
        {
            std::vector<code_fence> out;
            size_t i = 0, n = text.size();
            while (i < n)
            {
                if ((i == 0 || text[i - 1] == '\n') && text.compare(i, 3, "```") == 0)
                {
                    size_t fence_end = text.find('\n', i);
                    if (fence_end == std::string::npos) break;
                    std::string lang = text.substr(i + 3, fence_end - (i + 3));
                    while (!lang.empty() && isspace((unsigned char)lang.back())) lang.pop_back();

                    size_t search = fence_end + 1, close_start = std::string::npos, close_end = n;
                    while (search <= n)
                    {
                        size_t line_end = text.find('\n', search);
                        size_t this_end = (line_end == std::string::npos) ? n : line_end;
                        size_t last = text.find_last_not_of(" \t\r", this_end > search ? this_end - 1 : search);
                        bool is_close = (last != std::string::npos && last >= search &&
                            text.compare(search, last - search + 1, "```") == 0);
                        if (is_close) { close_start = search; close_end = this_end; break; }
                        if (line_end == std::string::npos) break;
                        search = line_end + 1;
                    }

                    if (close_start != std::string::npos)
                    {
                        code_fence f;
                        f.block_start = i;
                        f.block_end = std::min(close_end + 1, n);
                        f.lang = lang;
                        f.body = text.substr(fence_end + 1, close_start - (fence_end + 1));
                        if (!f.body.empty() && f.body.back() == '\n') f.body.pop_back();
                        out.push_back(std::move(f));
                        i = f.block_end;
                        continue;
                    }
                }
                i++;
            }
            return out;
        }

        // Header bar (language + copy-to-clipboard) with line-numbered, lightly syntax-colored,
        // non-wrapping code below it - the parts of the reference "code card" look imgui_md has no
        // hook for (see find_fenced_code_blocks above for why this bypasses imgui_md entirely).
        void draw_code_block(ux_window& win, const std::string& lang, const std::string& code, float wrap_width)
        {
            std::vector<std::string> lines;
            size_t start = 0;
            while (start <= code.size())
            {
                size_t nl = code.find('\n', start);
                lines.push_back(code.substr(start, (nl == std::string::npos ? code.size() : nl) - start));
                if (nl == std::string::npos) break;
                start = nl + 1;
            }

            ImGui::PushFont(win.get_font());
            const float line_h = ImGui::GetTextLineHeightWithSpacing();
            const float pad = ImGui::GetStyle().FramePadding.x;

            ImVec2 block_start = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->ChannelsSplit(2);
            dl->ChannelsSetCurrent(1);

            ImGui::Indent(pad);
            ImGui::Dummy(ImVec2(0.f, pad * 0.5f));

            std::string label = lang.empty() ? std::string("CODE") : lang;
            std::transform(label.begin(), label.end(), label.begin(), [](unsigned char c) { return (char)toupper(c); });
            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 0.6f));
            ImGui::TextUnformatted(label.c_str());
            ImGui::PopStyleColor();

            std::string copy_label = std::string(textual_icons::copy) + "  Copy";
            float copy_w = ImGui::CalcTextSize(copy_label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.f;
            float right_edge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
            ImGui::SameLine(right_edge - copy_w);
            ImGui::PushStyleColor(ImGuiCol_Button, transparent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, alpha(light_grey, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, alpha(light_grey, 0.25f));
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            if (ImGui::SmallButton(copy_label.c_str()))
                ImGui::SetClipboardText(code.c_str());
            if (ImGui::IsItemHovered())
                win.link_hovered();
            ImGui::PopStyleColor(4);

            ImGui::Dummy(ImVec2(0.f, pad * 0.5f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.f, pad * 0.5f));

            char gutter_buf[16];
            snprintf(gutter_buf, sizeof(gutter_buf), "%d", (int)lines.size());
            ImGui::PushFont(win.get_monofont());
            float gutter_w = ImGui::CalcTextSize(gutter_buf).x + pad;
            ImGui::PopFont();

            float lines_h = line_h * (float)lines.size() + pad;
            ImGui::BeginChild("##code_lines", ImVec2(wrap_width - pad * 2.f, lines_h),
                ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(win.get_monofont());
            for (size_t li = 0; li < lines.size(); li++)
            {
                snprintf(gutter_buf, sizeof(gutter_buf), "%d", (int)li + 1);
                ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 0.35f));
                ImGui::TextUnformatted(gutter_buf);
                ImGui::PopStyleColor();
                ImGui::SameLine(gutter_w);

                auto tokens = tokenize_code_line(lines[li]);
                for (size_t k = 0; k < tokens.size(); k++)
                {
                    if (k > 0) ImGui::SameLine(0.f, 0.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, tokens[k].color);
                    ImGui::TextUnformatted(tokens[k].text.c_str());
                    ImGui::PopStyleColor();
                }
                if (tokens.empty()) ImGui::NewLine();
            }
            ImGui::PopFont();
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.f, pad * 0.5f));
            ImGui::Unindent(pad);

            ImVec2 block_end(block_start.x + wrap_width, ImGui::GetCursorScreenPos().y);
            dl->ChannelsSetCurrent(0);
            dl->AddRectFilled(block_start, block_end, ImGui::ColorConvertFloat4ToU32(dark_grey), 4.f);
            dl->ChannelsMerge();

            ImGui::PopFont();
        }

        // Subclasses imgui_md to hook our custom behaviors into its CommonMark+GFM rendering:
        // link/image clicks go through the existing open_url(), inline images/gifs are bridged to
        // the existing assistant_image_cache's async fetch/decode/GL-upload pipeline, and code gets
        // a monospace font + (for fenced blocks) a background rect, none of which imgui_md styles
        // on its own. Constructed fresh per draw_markdown_body() call - it holds no parse state.
        struct assistant_markdown_renderer : public imgui_md
        {
            assistant_markdown_renderer(ux_window& win, assistant::assistant_image_cache& images,
                const assistant::invoke_fn& invoke, float wrap_width, int& table_index)
                : _win(win), _images(images), _invoke(invoke), _wrap_width(wrap_width), _table_index(table_index) {}

        protected:
            ImFont* get_font() const override
            {
                if (m_is_code) return _win.get_monofont();
                if (m_hlevel > 0) return _win.get_large_font();
                if (m_is_strong || _in_table_header) return _win.get_bold_font();
                return _win.get_font();
            }

            ImVec4 get_color() const override
            {
                if (!m_href.empty()) return light_blue;
                return light_grey;
            }

            void open_url() const override
            {
                if (!m_href.empty())
                    rs2::open_url(m_href.c_str());
            }

            // Inline `code` - imgui_md's default SPAN_CODE is a no-op, so without this override
            // inline code renders in the surrounding paragraph's font/color with no visual distinction.
            void SPAN_CODE(bool e) override
            {
                m_is_code = e;
                if (e)
                {
                    ImGui::PushFont(get_font());
                    ImGui::PushStyleColor(ImGuiCol_Text, get_color());
                }
                else
                {
                    ImGui::PopStyleColor();
                    ImGui::PopFont();
                }
            }

            // Fenced code blocks - same font/color gap as SPAN_CODE, plus a background rect drawn
            // behind the block via channel splitting (the rect's extent isn't known until the block's
            // content has been laid out, so it must be drawn after the fact, behind what's already there).
            void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL*, bool e) override
            {
                if (e)
                {
                    m_is_code = true;
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->ChannelsSplit(2);
                    dl->ChannelsSetCurrent(1);
                    _code_block_start = ImGui::GetCursorScreenPos();
                    ImGui::PushFont(get_font());
                    ImGui::PushStyleColor(ImGuiCol_Text, get_color());
                }
                else
                {
                    ImGui::PopStyleColor();
                    ImGui::PopFont();

                    ImVec2 end(_code_block_start.x + ImGui::GetContentRegionAvail().x,
                        ImGui::GetCursorScreenPos().y);
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->ChannelsSetCurrent(0);
                    dl->AddRectFilled(_code_block_start, end, ImGui::ColorConvertFloat4ToU32(dark_grey), 3.f);
                    dl->ChannelsMerge();

                    m_is_code = false;
                }
            }

            // Real ImGui::BeginTable, not imgui_md's own (which drifts columns out of alignment
            // across rows). Wrapped in our own auto-height child rather than BeginTable's ScrollX,
            // so a wide table scrolls horizontally without us guessing its height up front.
            void BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL* d, bool e) override
            {
                if (e)
                {
                    // IDs carry a per-message table index - a message with 2+ tables would otherwise
                    // have every table share one ImGui ID, causing scroll/layout state conflicts.
                    char wrap_id[32], table_id[32];
                    snprintf(wrap_id, sizeof(wrap_id), "##table_wrap_%d", _table_index);
                    snprintf(table_id, sizeof(table_id), "md_table_%d", _table_index);
                    _table_index++;

                    ImGui::NewLine(); // trailing text may leave the cursor mid-line - see render_text()'s SameLine(0,0)
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, transparent);
                    ImGui::BeginChild(wrap_id, ImVec2(_wrap_width, 0.f),
                        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize,
                        ImGuiWindowFlags_HorizontalScrollbar);
                    // Without ScrollX/ScrollY, BeginTable auto-fits height to content, but also
                    // auto-fits WIDTH to the wrapping child unless given an explicit outer width -
                    // which would silently shrink our fixed columns instead of scrolling them.
                    const float col_w = 130.f;
                    // BeginTable can fail (e.g. window clipped) - EndTable() must only be called,
                    // and no table functions submitted, when it returned true.
                    _table_open = ImGui::BeginTable(table_id, (int)d->col_count,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
                        ImVec2((float)d->col_count * col_w, 0.f));
                    if (_table_open)
                        for (unsigned i = 0; i < d->col_count; i++)
                            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, col_w);
                }
                else
                {
                    if (_table_open) ImGui::EndTable();
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                    ImGui::NewLine();
                }
            }

            // Tracked separately from base's m_is_table_header: that flag also disables word-wrap
            // in the private render_text(), fine for imgui_md's own table but clips our fixed-width
            // columns' header text instead of wrapping it.
            void BLOCK_THEAD(bool e) override
            {
                _in_table_header = e;
                if (e) ImGui::PushFont(get_font());
                else ImGui::PopFont();
            }

            // Deliberately left unset: base's m_is_table_body makes render_text() use column-
            // position bookkeeping we never populate (our BeginTable overrides bypass it), wrapping
            // body cells after a single character instead of to the column's real width.
            void BLOCK_TBODY(bool) override {}

            void BLOCK_TR(bool e) override
            {
                if (e && _table_open) ImGui::TableNextRow();
            }

            void BLOCK_TH(const MD_BLOCK_TD_DETAIL* d, bool e) override { BLOCK_TD(d, e); }

            void BLOCK_TD(const MD_BLOCK_TD_DETAIL*, bool e) override
            {
                if (e && _table_open) ImGui::TableNextColumn();
            }

            // Fully overridden (rather than just get_image()) so a still-loading or failed fetch has
            // something to show - imgui_md's default SPAN_IMG draws nothing at all when get_image()
            // returns false, silently skipping the image line.
            void SPAN_IMG(const MD_SPAN_IMG_DETAIL* d, bool e) override
            {
                m_is_image = e;
                if (e) m_href.assign(d->src.text, d->src.size);

                if (e)
                {
                    auto* img = _images.get_or_load(m_href, _invoke);
                    if (img->state == assistant::image_load_state::loading)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 0.6f));
                        ImGui::TextUnformatted("Loading image...");
                        ImGui::PopStyleColor();
                    }
                    else if (img->state == assistant::image_load_state::failed)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui::TextUnformatted(m_href.c_str());
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemClicked()) open_url();
                        if (ImGui::IsItemHovered())
                        {
                            RsImGui::CustomTooltip(m_href.c_str());
                            _win.link_hovered();
                        }
                    }
                    else // loaded
                    {
                        image_info nfo;
                        if (get_image(nfo))
                        {
                            ImVec2 const csz = ImGui::GetContentRegionAvail();
                            if (nfo.size.x > csz.x)
                            {
                                const float r = nfo.size.y / nfo.size.x;
                                nfo.size.x = csz.x;
                                nfo.size.y = csz.x * r;
                            }
                            ImGui::Image(nfo.texture_id, nfo.size, nfo.uv0, nfo.uv1, nfo.col_tint, nfo.col_border);
                            if (ImGui::IsItemClicked()) open_url();
                            if (ImGui::IsItemHovered())
                            {
                                RsImGui::CustomTooltip(m_href.c_str());
                                _win.link_hovered();
                            }
                        }
                    }
                }
                else
                {
                    m_href.clear();
                }
            }

            bool get_image(image_info& nfo) const override
            {
                auto* img = _images.get_or_load(m_href, _invoke);
                if (img->state != assistant::image_load_state::loaded)
                    return false;

                size_t frame = current_frame_index(*img, _win.time());
                nfo.texture_id = (ImTextureID)(intptr_t)img->frame_textures[frame]->get_gl_handle();
                nfo.size = ImVec2((float)img->width, (float)img->height);
                nfo.uv0 = ImVec2(0.f, 0.f);
                nfo.uv1 = ImVec2(1.f, 1.f);
                nfo.col_tint = ImVec4(1.f, 1.f, 1.f, 1.f);
                nfo.col_border = ImVec4(0.f, 0.f, 0.f, 0.f);
                return true;
            }

        private:
            ux_window& _win;
            assistant::assistant_image_cache& _images;
            const assistant::invoke_fn& _invoke;
            float _wrap_width;
            bool _in_table_header = false;
            bool _table_open = false;
            // Reference, not owned: shared across every renderer instance draw_markdown_body()
            // constructs for one message (a fresh instance per prose segment between fenced code
            // blocks), so table IDs keep accumulating instead of restarting at 0 per segment.
            int& _table_index;
            ImVec2 _code_block_start;
        };
    }

    namespace assistant_detail
    {
        void draw_markdown_body(ux_window& win, const std::string& text, float wrap_width,
            assistant::assistant_image_cache& images, const assistant::invoke_fn& invoke)
        {
            // imgui_md wraps at the current window's content-region width, so a child of exactly
            // wrap_width - auto-sized to content height - is what makes it wrap where the caller wants
            // rather than at the full (unindented) message-list width.
            ImGui::PushStyleColor(ImGuiCol_ChildBg, transparent);
            ImGui::BeginChild("##md_body", ImVec2(wrap_width, 0.f),
                ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);

            auto fences = find_fenced_code_blocks(text);
            size_t pos = 0;
            int block_index = 0;
            int table_index = 0; // shared across every prose segment, so table IDs don't reset per segment
            for (auto&& f : fences)
            {
                if (f.block_start > pos)
                {
                    std::string prose = text.substr(pos, f.block_start - pos);
                    assistant_markdown_renderer renderer(win, images, invoke, wrap_width, table_index);
                    renderer.print(prose.data(), prose.data() + prose.size());
                }
                ImGui::PushID(block_index++);
                draw_code_block(win, f.lang, f.body, wrap_width);
                ImGui::PopID();
                pos = f.block_end;
            }
            if (pos < text.size())
            {
                std::string prose = text.substr(pos);
                assistant_markdown_renderer renderer(win, images, invoke, wrap_width, table_index);
                renderer.print(prose.data(), prose.data() + prose.size());
            }

            ImGui::PopStyleColor();

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }
}
