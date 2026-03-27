#ifndef COMPOSESIREN_CPP_STDLIB_IS_CRIPPLED_H
#define COMPOSESIREN_CPP_STDLIB_IS_CRIPPLED_H

#include <map>

template<typename k, typename v> bool mapTryGet(const std::map<k,v> &map, const k &key, v &value) {
    if(auto search = map.find(key); search != map.end()){
        value = search->second;
        return true;
    }
    return false;
}

#endif // COMPOSESIREN_CPP_STDLIB_IS_CRIPPLED_H
