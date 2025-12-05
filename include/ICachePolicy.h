#pragma once


namespace bananaWebServer
{

template<typename Key, typename Value>
class ICachePolicy
{
    virtual ~ICachePolicy() = 0;
    virtual bool get(Key key, Value &value) = 0;
    virtual Value get(Key key) = 0;
    virtual void put(Key key, Value value) - 0;
};


} // namespace bananaWebServer

