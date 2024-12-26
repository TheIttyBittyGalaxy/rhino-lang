#define WITH_COMMA(token) token,

#define RETURN_STRING_IF_MATCH(token) \
    if (value == token)               \
        return #token;
