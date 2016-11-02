// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include "third_party/rapidxml/rapidxml.hpp"

#include <vector>
#include <algorithm>
#include <cstring>
#include <map>
#include <ctype.h>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>

#pragma pack(push, 1)
typedef struct
{
    uint8_t     Version_Revision;
    uint8_t     Version_Number;
    uint8_t     Version_Minor;
    uint8_t     Version_Major;
    int         Version_spare;
}ChangeSetVersion;

typedef struct
{
    uint8_t     Version_Minor;
    uint8_t     Version_Major;
    uint8_t     Version_spare[2];
}MajorMinorVersion;

typedef struct
{
    uint8_t     Version1;
    uint8_t     Version2;
    uint8_t     Version3;
    uint8_t     Version4;
}HexByte;

typedef struct
{
    uint8_t     Version2;
    uint8_t     Version1;
    uint8_t     Version_spare[2];
}LiguriaVersion;

typedef struct
{
    uint8_t     sts;
}Bool;

typedef struct
{
    uint8_t     num;
}HwTypeNumber;

typedef struct
{
    uint8_t     Version_Minor;
    uint8_t     Version_Major;
    uint8_t     Version_spare[2];
}WideMajorMinorVersion;

typedef struct
{
    uint8_t     value[32];
}Ascii;

typedef struct
{
    uint8_t     version;
}DecByte;

typedef struct
{
    uint8_t     number1;
    uint8_t     number2;
    uint8_t     number3;
    uint8_t     number4;
}HexNumber;

typedef struct
{
    uint8_t     number1;
    uint8_t     number2;
    uint8_t     number3;
    uint8_t     number4;
}HexNumberReversed;

typedef struct
{
    uint8_t     number1;
    uint8_t     number2;
    uint8_t     number3;
    uint8_t     number4;
    uint8_t     number5;
    uint8_t     number6;
    uint8_t     number7;
    uint8_t     number8;
}BarCodeSerial12Char;

typedef struct
{
    double      number;
}DoubleNumber;
#pragma pack(pop)

struct parameter {
    std::string name, data;
    bool is_decimal, is_reverse_bytes;
    int format_length;

    parameter(std::string in_name, std::string in_data,
        bool in_is_decimal, bool in_is_reverse_bytes,
            int in_format_length)
    : name(in_name),
      format_length(in_format_length),
      is_decimal(in_is_decimal),
      is_reverse_bytes(in_is_reverse_bytes),
      data(in_data)
    {}
    parameter()
    : name(""),
      format_length(2),
      is_decimal(false),
      is_reverse_bytes(false),
      data("")
    {}
};

struct section {
    std::string name, title, format_type, data;
    int offset, size;
};

struct data {
    std::vector<section> sections;
};

struct kvp {
    std::string key, value;
};

struct custom_formatter {
    std::string name, key_size;
    std::vector<kvp> kv;
};

struct command
{
    std::string name;
    unsigned int op_code;
    std::string read_format;
    bool is_write_only = false;
    bool is_read_command = false;
    std::string description;
    int time_out = 20000;
    int num_of_parameters = 0;
    bool is_cmd_write_data = false;
    std::string cmd_permission;
    std::string cmd_interface;
    std::string cmd_pipe;
    std::string i2c_reg_offset;
    std::string i2c_cmd_type;

    data read_data;
    std::vector<parameter> parameters;
};

// units of nibbles
enum FormatSize {
    Byte = 2,
    Word = 4,
    Double = 8
};

struct commands_xml
{
    std::map<std::string, command> commands;
    std::map<std::string, custom_formatter> custom_formatters;
};

inline std::string hexify(unsigned char n)
{
    std::string res;

    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);

    std::reverse(res.begin(), res.end());

    if (res.size() == 1)
    {
        res.insert(0, "0");
    }

    return res;
}

inline void split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    auto pos = txt.find(ch);
    auto initial_pos = 0;
    strs.clear();

    while (pos != std::string::npos) {
        strs.push_back(txt.substr(initial_pos, pos - initial_pos + 1));
        initial_pos = pos + 1;

        pos = txt.find(ch, initial_pos);
    }

    strs.push_back(txt.substr(initial_pos, std::min(pos, txt.size()) - initial_pos + 1));
}

inline std::string to_lower(std::string x)
{
    transform(x.begin(), x.end(), x.begin(), tolower);
    return x;
}

inline bool string_to_bool(const std::string& x)
{
    return (to_lower(x) == "true");
}

inline unsigned int string_to_hex(std::string str)
{
    std::string delimiter = "x";

    auto pos = str.find(delimiter);
    str.erase(0, pos + delimiter.length());

    std::stringstream ss;
    unsigned int value;
    ss << str;
    ss >> std::hex >> value;

    return value;
}

inline void parse_xml_from_memory(const char * content, commands_xml& cmd_xml)
{
    auto doc = std::make_shared<rapidxml::xml_document<>>();
    doc->parse<0>(doc->allocate_string(content));

    auto node = doc->first_node("Commands");
    if (!node)
        throw std::logic_error("Corrupted XML file");

    for (auto NodeI = node->first_node(); NodeI; NodeI = NodeI->next_sibling())
    {
        if (!strcmp(NodeI->name(), "Command"))
        {
            command cmd;
            for (auto AttI = NodeI->first_attribute(); AttI; AttI = AttI->next_attribute())
            {
                std::string value = AttI->value();
                std::string att_name = AttI->name();
                if (att_name == "Name") { cmd.name = value; }
                else if (att_name == "Opcode") { cmd.op_code = string_to_hex(value); }
                else if (att_name == "IsWriteOnly") { cmd.is_write_only = string_to_bool(value); }
                else if (att_name == "ReadFormat") { cmd.read_format = value; }
                else if (att_name == "IsReadCommand") { cmd.is_read_command = string_to_bool(value); }
                else if (att_name == "CmdPermission") { cmd.cmd_permission = value; }
                else if (att_name == "TimeOut") { cmd.time_out = stoi(value); }
                else if (att_name == "Description") { cmd.description = value; }
                else if (att_name == "CmdInterface") { cmd.cmd_interface = value; }
                else if (att_name == "CmdPipe") { cmd.cmd_pipe = value; }
                else if (att_name == "I2cRegOffset") { cmd.i2c_reg_offset = value; }
                else if (att_name == "I2cCmdType") { cmd.i2c_cmd_type = value; }
            }

            for (auto SubNodeI = NodeI->first_node(); SubNodeI; SubNodeI = SubNodeI->next_sibling()) // Add commands
            {
                std::string str_name = SubNodeI->name();
                if (str_name.find("Parameter") != std::string::npos)
                {
                    parameter param;
                    cmd.num_of_parameters++;
                    for (auto paramAtt = SubNodeI->first_attribute(); paramAtt; paramAtt = paramAtt->next_attribute())
                    {
                        std::string value = paramAtt->value();
                        std::string att_name = paramAtt->name();
                        if (att_name == "Name") { param.name = value; }
                        else if (att_name == "IsDecimal") { param.is_decimal = string_to_bool(value); }
                    }
                    cmd.parameters.push_back(param);
                }
                else if (str_name == "Data")
                {
                    parameter param;
                    param.name = "Data";
                    cmd.is_cmd_write_data = true;
                    for (auto dataAtt = SubNodeI->first_attribute(); dataAtt; dataAtt = dataAtt->next_attribute())
                    {
                        std::string value = dataAtt->value();
                        std::string data_att_name = dataAtt->name();

                        if (data_att_name == "Name") { param.name = value; }
                        else if (data_att_name == "IsReverseBytes") { param.is_reverse_bytes = string_to_bool(value); }
                        else if (data_att_name == "FormatLength") { param.format_length = stoi(value); }
                    }
                    cmd.parameters.push_back(param);
                }
                else if (str_name == "ReadData")
                {
                    for (auto sectionNode = SubNodeI->first_node(); sectionNode; sectionNode = sectionNode->next_sibling()) // Add CMD Parameters
                    {
                        section sec;
                        for (auto sectionAtt = sectionNode->first_attribute(); sectionAtt; sectionAtt = sectionAtt->next_attribute())
                        {
                            std::string value = sectionAtt->value();
                            std::string section_att_name = sectionAtt->name();
                            if (section_att_name == "Name") { sec.name = value; }
                            else if (section_att_name == "Title") { sec.title = value; }
                            else if (section_att_name == "Offset") { sec.offset = stoi(value); }
                            else if (section_att_name == "Size") { sec.size = stoi(value); }
                            else if (section_att_name == "FormatType") { sec.format_type = value; }
                        }
                        cmd.read_data.sections.push_back(sec);
                    }
                }
            }
            cmd_xml.commands.insert(std::pair<std::string, command>(to_lower(cmd.name), cmd));
        }
        else if (!strcmp(NodeI->name(), "CustomFormatter"))
        {
            custom_formatter customFormatter;
            for (auto AttI = NodeI->first_attribute(); AttI; AttI = AttI->next_attribute())
            {
                std::string value = AttI->value();
                std::string atti_name = AttI->name();
                if (atti_name == "KeySize") { customFormatter.key_size = value; }
                else if (atti_name == "Name") { customFormatter.name = value; }
            }

            for (auto SubNodeI = NodeI->first_node(); SubNodeI; SubNodeI = SubNodeI->next_sibling()) // Add Kvp
            {
                std::string sub_nodei_name = SubNodeI->name();
                if (sub_nodei_name.find("KVP") != std::string::npos)
                {
                    kvp kvp;
                    for (auto paramAtt = SubNodeI->first_attribute(); paramAtt; paramAtt = paramAtt->next_attribute())
                    {
                        std::string value = paramAtt->value();
                        std::string param_att_name = paramAtt->name();
                        if (param_att_name == "Key") { kvp.key = value; }
                        else if (param_att_name == "Value") { kvp.value = value; }
                    }
                    customFormatter.kv.push_back(kvp);
                }

            }
            cmd_xml.custom_formatters.insert(std::pair<std::string, custom_formatter>(to_lower(customFormatter.name), customFormatter));
        }
    }
}

inline bool parse_xml_from_file(const char* xml_full_file_path, commands_xml& cmd_xml)
{
    std::ifstream fin(xml_full_file_path);

    if (!fin)
        return false;

    std::stringstream ss;
    ss << fin.rdbuf();
    auto xml = ss.str();

    parse_xml_from_memory(xml.c_str(), cmd_xml);

    return true;
}

inline void make_depth_histogram(uint8_t rgb_image[], const uint16_t depth_image[], int width, int height)
{
    static uint32_t histogram[0x10000];
    memset(histogram, 0, sizeof(histogram));

    for (auto i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
    for (auto i = 2; i < 0x10000; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    for (auto i = 0; i < width*height; ++i)
    {
        if (auto d = depth_image[i])
        {
            int f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
            rgb_image[i * 3 + 0] = 255 - f;
            rgb_image[i * 3 + 1] = 0;
            rgb_image[i * 3 + 2] = f;
        }
        else
        {
            rgb_image[i * 3 + 0] = 20;
            rgb_image[i * 3 + 1] = 5;
            rgb_image[i * 3 + 2] = 0;
        }
    }
}


inline float clamp(float x, float min, float max)
{
    return std::max(std::min(max, x), min);
}

inline float smoothstep(float x, float min, float max)
{
    x = clamp((x - min) / (max - min), 0.0, 1.0);
    return x*x*(3 - 2 * x);
}

inline float lerp(float a, float b, float t)
{
    return b * t + a * (1 - t);
}

struct float2
{
    float x, y;
};
struct rect
{
    float x, y;
    float w, h;

    bool operator==(const rect& other) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }

    rect center() const
    {
        return{ x + w / 2, y + h / 2, 0, 0 };
    }

    rect lerp(float t, const rect& other) const
    {
        return{
            ::lerp(x, other.x, t), ::lerp(y, other.y, t),
            ::lerp(w, other.w, t), ::lerp(h, other.h, t),
        };
    }

    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
};

////////////////////////
// Image display code //
////////////////////////

class texture_buffer
{
    GLuint texture;
    std::vector<uint8_t> rgb;

public:
    texture_buffer() : texture() {}

    GLuint get_gl_handle() const { return texture; }

    void upload(const void * data, int width, int height, rs_format format, int stride = 0)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if(!texture) glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        stride = stride == 0 ? width : stride;
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
        switch(format)
        {
        case RS_FORMAT_ANY:
        throw std::runtime_error("not a valid format");
        case RS_FORMAT_Z16:
        case RS_FORMAT_DISPARITY16:
            rgb.resize(640 * 480 * 4);
            make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            
            break;
        case RS_FORMAT_XYZ32F:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
            break;
        case RS_FORMAT_YUYV: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGB8: case RS_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_Y16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;
        case RS_FORMAT_RAW8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RAW10:
            {
                // Visualize Raw10 by performing a naive downsample. Each 2x2 block contains one red pixel, two green pixels, and one blue pixel, so combine them into a single RGB triple.
                rgb.clear(); rgb.resize(width/2 * height/2 * 3);
                auto out = rgb.data(); auto in0 = reinterpret_cast<const uint8_t *>(data), in1 = in0 + width*5/4;
                for(auto y=0; y<height; y+=2)
                {
                    for(auto x=0; x<width; x+=4)
                    {
                        *out++ = in0[0]; *out++ = (in0[1] + in1[0]) / 2; *out++ = in1[1]; // RGRG -> RGB RGB
                        *out++ = in0[2]; *out++ = (in0[3] + in1[2]) / 2; *out++ = in1[3]; // GBGB
                        in0 += 5; in1 += 5;
                    }
                    in0 = in1; in1 += width*5/4;
                }
                glPixelStorei(GL_UNPACK_ROW_LENGTH, width / 2);        // Update row stride to reflect post-downsampling dimensions of the target texture
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width/2, height/2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            }
            break;
        default:
            throw std::runtime_error("The requested format is not provided by demo");
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void upload(rs::frame& frame)
    {
        upload(frame.get_data(), frame.get_width(), frame.get_height(), frame.get_format(), 
            (frame.get_stride_in_bytes() * 8) / frame.get_bits_per_pixel());
    }

    void show(const rect& r, float alpha = 1.0f) const
    {
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x, r.y + r.h);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(r.x, r.y);
        glTexCoord2f(1, 0); glVertex2f(r.x + r.w, r.y);
        glTexCoord2f(1, 1); glVertex2f(r.x + r.w, r.y + r.h);
        glTexCoord2f(0, 1); glVertex2f(r.x, r.y + r.h);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDisable(GL_BLEND);
    }
};

inline void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
    static uint8_t rgb_image[640*480*3];
    make_depth_histogram(rgb_image, depth_image, width, height);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
}
