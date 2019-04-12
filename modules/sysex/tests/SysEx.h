#pragma once

#include "../src/SysEx.h"

#define NUMBER_OF_BLOCKS                    1
#define NUMBER_OF_SECTIONS                  3

#define SECTION_0_PARAMETERS                10
#define SECTION_1_PARAMETERS                6
#define SECTION_2_PARAMETERS                33

#define SECTION_0_MIN                       0
#define SECTION_0_MAX                       50

#define SECTION_1_MIN                       0
#define SECTION_1_MAX                       0

#define SECTION_2_MIN                       0
#define SECTION_2_MAX                       0

#define TEST_BLOCK_ID                       0

#define TEST_SECTION_SINGLE_PART_ID         0
#define TEST_SECTION_MULTIPLE_PARTS_ID      2

#define TEST_SECTION_NOMINMAX               1
#define TEST_INDEX_ID                       5
#define TEST_MSG_PART_VALID                 0
#define TEST_MSG_PART_INVALID               10
#define TEST_INVALID_PARAMETER_B0S0         15
#define TEST_NEW_VALUE_VALID                25
#define TEST_NEW_VALUE_INVALID              100

#define TEST_VALUE_GET                      3

#define CUSTOM_REQUEST_ID_VALID             54
#define CUSTOM_REQUEST_ID_INVALID           55
#define CUSTOM_REQUEST_ID_ERROR_READ        56
#define CUSTOM_REQUEST_ID_NO_CONN_CHECK     57
#define CUSTOM_REQUEST_VALUE                1
#define TOTAL_CUSTOM_REQUESTS               3

#define ON_GET_VALID                                                                            \
bool onGet(uint8_t block, uint8_t section, uint16_t index, sysExParameter_t &value) override    \
{                                                                                               \
    if (userError)                                                                              \
    {                                                                                           \
        setError(userError);                                                                    \
        return false;                                                                           \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        value = TEST_VALUE_GET;                                                                 \
        return true;                                                                            \
    }                                                                                           \
}                                                                                               \

#define ON_GET_INVALID                                                                          \
bool onGet(uint8_t block, uint8_t section, uint16_t index, sysExParameter_t &value) override    \
{                                                                                               \
    return false;                                                                               \
}                                                                                               \

#define ON_SET_VALID                                                                            \
bool onSet(uint8_t block, uint8_t section, uint16_t index, sysExParameter_t newValue) override  \
{                                                                                               \
    if (userError)                                                                              \
    {                                                                                           \
        setError(userError);                                                                    \
        return false;                                                                           \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        return true;                                                                            \
    }                                                                                           \
}                                                                                               \

#define ON_SET_INVALID                                                                          \
bool onSet(uint8_t block, uint8_t section, uint16_t index, sysExParameter_t newValue) override  \
{                                                                                               \
    return false;                                                                               \
}                                                                                               \

class SysExTestingValid : public SysEx
{
    public:
    SysExTestingValid()
    {
        responseCounter = 0;
        userError = REQUEST;
    }

    ~SysExTestingValid() {}

    int responseCounter;
    sysExStatus_t userError;
    uint8_t testArray[200];

    bool onCustomRequest(uint8_t value)
    {
        switch(value)
        {
            case CUSTOM_REQUEST_ID_VALID:
            case CUSTOM_REQUEST_ID_NO_CONN_CHECK:
            addToResponse(CUSTOM_REQUEST_VALUE);
            return true;
            break;

            case CUSTOM_REQUEST_ID_ERROR_READ:
            default:
            return false;
            break;
        }
    }

    ON_GET_VALID

    ON_SET_VALID

    void onWrite(uint8_t testArray[], uint8_t arraysize)
    {
        for (int i=0; i<arraysize; i++)
            this->testArray[i] = testArray[i];

        responseCounter++;
    }
};

class SysExTestingErrorGet : public SysEx
{
    public:
    SysExTestingErrorGet()
    {
        responseCounter = 0;
        userError = REQUEST;
    }

    ~SysExTestingErrorGet() {}
    int responseCounter;
    sysExStatus_t userError;
    uint8_t testArray[200];

    bool onCustomRequest(uint8_t value) override
    {
        switch(value)
        {
            case CUSTOM_REQUEST_ID_VALID:
            case CUSTOM_REQUEST_ID_NO_CONN_CHECK:
            addToResponse(CUSTOM_REQUEST_VALUE);
            return true;
            break;

            case CUSTOM_REQUEST_ID_ERROR_READ:
            default:
            return false;
            break;
        }
    }

    ON_GET_INVALID

    ON_SET_VALID

    void onWrite(uint8_t testArray[], uint8_t arraysize) override
    {
        for (int i=0; i<arraysize; i++)
            this->testArray[i] = testArray[i];

        responseCounter++;
    }
};

class SysExTestingErrorSet : public SysEx
{
    public:
    SysExTestingErrorSet()
    {
        responseCounter = 0;
        userError = REQUEST;
    }

    ~SysExTestingErrorSet() {}
    int responseCounter;
    sysExStatus_t userError;
    uint8_t testArray[200];

    bool onCustomRequest(uint8_t value) override
    {
        switch(value)
        {
            case CUSTOM_REQUEST_ID_VALID:
            case CUSTOM_REQUEST_ID_NO_CONN_CHECK:
            addToResponse(CUSTOM_REQUEST_VALUE);
            return true;
            break;

            case CUSTOM_REQUEST_ID_ERROR_READ:
            default:
            return false;
            break;
        }
    }

    ON_GET_VALID

    ON_SET_INVALID

    void onWrite(uint8_t testArray[], uint8_t arraysize) override
    {
        for (int i=0; i<arraysize; i++)
            this->testArray[i] = testArray[i];

        responseCounter++;
    }
};