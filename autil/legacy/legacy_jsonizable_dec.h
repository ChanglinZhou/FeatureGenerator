#pragma once

#include <stdint.h>
#include <map>
#include <string>

#include "autil/legacy/any.h"
#include "autil/legacy/json.h"
#include "autil/legacy/fast_jsonizable_dec.h"
#include "autil/legacy/legacy_jsonizable.h"

namespace autil {
namespace legacy {

class Jsonizable : public FastJsonizableBase
{
public:
    static constexpr int FAST_MODE_MAGIC_NUMBER = 31415926;
    using FastJsonizableBase::Mode;

    class JsonWrapper : public FastJsonizable::JsonWrapper
    {
    public:
        JsonWrapper()
            : mFastMagicNum(0)
        {}

        JsonWrapper(const Any& json)
            : mFastMagicNum(0)
        {
            mMode = FROM_JSON;
            mJsonMap = AnyCast<std::map<std::string, Any>>(json);
        }

        JsonWrapper(RapidWriter *writer)
            : FastJsonizable::JsonWrapper(writer)
            , mFastMagicNum(FAST_MODE_MAGIC_NUMBER)
        {}

        JsonWrapper(RapidValue *value)
            : FastJsonizable::JsonWrapper(value)
            , mFastMagicNum(FAST_MODE_MAGIC_NUMBER)
        {}

        int32_t GetFastMagicNum() const
        { return mFastMagicNum; }

        std::map<std::string, Any> GetMap()
        { return mJsonMap; }

        template<typename T>
        void Jsonize(const std::string& key, T& value) {
            if (mFastMagicNum == FAST_MODE_MAGIC_NUMBER) {
                FastJsonizable::JsonWrapper::Jsonize(key, value);
            } else {
                AnyJsonize(key, value);
            }
        }

        template<typename T>
        void Jsonize(const std::string& key, T& value, const T& defaultValue) {
            if (mFastMagicNum == FAST_MODE_MAGIC_NUMBER) {
                FastJsonizable::JsonWrapper::Jsonize(key, value, defaultValue);
            } else {
                AnyJsonize(key, value, defaultValue);
            }
        }

        inline void JsonizeAsString(const std::string& key, std::string& value);
        inline void JsonizeAsString(const std::string &key, std::string &value,
                const std::string &defautValue);

    private:
        template<typename T>
        void AnyJsonize(const std::string& key, T& value);

        template<typename T>
        void AnyJsonize(const std::string& key, T& value, const T& defaultValue);

        void AnyJsonize(const std::string& key, RapidValue *&value) {
            AUTIL_LEGACY_THROW(NotJsonizableException, "any jsonize do not support RapidValue, key: " + key);
        }

    private:
        std::map<std::string, Any> mJsonMap;
        // avoid wrong call, assume a lib is compiled by old autil, that means
        // it's jsonwrapper do not have mFastMagicNum field, the access will get
        // an undefined value;
        int32_t mFastMagicNum;
    };

    virtual ~Jsonizable() {}
    virtual void Jsonize(JsonWrapper& json) = 0;
};

template<typename T>
void Jsonizable::JsonWrapper::AnyJsonize(const std::string& key, T& value)
{
    if (mMode == TO_JSON)
        mJsonMap[key] = ToJson(value);
    else
    {
        std::map<std::string, Any>::const_iterator it = mJsonMap.find(key);
        if (it == mJsonMap.end())
        {
            std::stringstream ss;
            ss << "context:{";
            for (std::map<std::string, Any>::const_iterator it = mJsonMap.begin();
                 it != mJsonMap.end();
                 it++)
            {
                ss << "," << it->first;
            }
            ss << "}";
            AUTIL_LEGACY_THROW(NotJsonizableException, key +
                    " not found when try to parse from Json." + ss.str());
        }
        FromJson(value, it->second);
    }
}

template<typename T>
void Jsonizable::JsonWrapper::AnyJsonize(
        const std::string& key, T& value, const T& defaultValue)
{
    if (mMode == TO_JSON)
        mJsonMap[key] = ToJson(value);
    else
    {
        std::map<std::string, Any>::const_iterator it = mJsonMap.find(key);
        if (it == mJsonMap.end())
            value = defaultValue;
        else
            FromJson(value, it->second);
    }
}

inline void Jsonizable::JsonWrapper::JsonizeAsString(const std::string &key, std::string &value) {
    if (mFastMagicNum == FAST_MODE_MAGIC_NUMBER) {
        return FastJsonizable::JsonWrapper::JsonizeAsString(key, value);
    } else {
        autil::legacy::Any anyVal;
        AnyJsonize(key, anyVal);
        value = ToJsonString(anyVal);
    }
}

inline void Jsonizable::JsonWrapper::JsonizeAsString(const std::string &key, std::string &value,
        const std::string &defaultValue)
{
    if (mFastMagicNum == FAST_MODE_MAGIC_NUMBER) {
        return FastJsonizable::JsonWrapper::JsonizeAsString(key, value, defaultValue);
    } else {
        autil::legacy::Any anyVal;
        AnyJsonize(key, anyVal, anyVal);
        if (!anyVal.IsEmpty()) {
            value = ToJsonString(anyVal);
        } else {
            value = defaultValue;
        }
    }
}

inline Any ToJson(const Jsonizable& t)
{
    Jsonizable::JsonWrapper w;
    const_cast<Jsonizable&>(t).Jsonize(w);
    return w.GetMap();
}

inline void FromJson(Jsonizable& t, const Any& a)
{
    Jsonizable::JsonWrapper w(a);
    t.Jsonize(w);
}

inline void FromRapidValue(Jsonizable& t, RapidValue& value) {
    if (!value.IsObject()) {
        AUTIL_LEGACY_THROW(TypeNotMatchException,
                           "type not match, expect Object but get: " + FastToJsonString(value));
    }
    Jsonizable::JsonWrapper w(&value);
    t.Jsonize(w);
}

inline void serializeToWriter(RapidWriter *writer, const Jsonizable &t) {
    Jsonizable::JsonWrapper w(writer);
    writer->StartObject();
    const_cast<Jsonizable&>(t).Jsonize(w);
    writer->EndObject();
}

}
}
