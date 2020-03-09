
enum RsRtspReturnCode
{
    OK,
    ERROR_GENERAL,
    ERROR_NETWROK,
    ERROR_TIME_OUT,
    ERROR_WRONG_FLOW
};

struct RsRtspReturnValue
{
    RsRtspReturnCode exit_code;
    std::string msg;
};

