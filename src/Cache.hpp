/*
Project: SSBRenderer
File: Cache.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <deque>
#include <algorithm>

template<typename Key, typename Value>
class Cache{
    private:
        std::deque<std::pair<Key,Value>> data;
        const unsigned int max_size;
    public:
        Cache() : max_size(64){}
        Cache(unsigned int max_size) : max_size(max_size){}
        bool contains(Key key){
            return std::find_if(this->data.begin(), this->data.end(), [&key](std::pair<Key,Value>& entry){
                return entry.first == key;
            }) != this->data.end();
        }
        Value get(Key key){
            auto it = std::find_if(this->data.begin(), this->data.end(), [&key](std::pair<Key,Value>& entry){
                return entry.first == key;
            });
            if(it != this->data.end()){
                auto elem = *it;
                this->data.erase(it);
                this->data.push_front(elem);
                return elem.second;
            }else
                return Value();
        }
        void add(Key key, Value value){
            this->data.push_front({key, value});
            if(this->data.size() > this->max_size)
                this->data.pop_back();
        }
        void clear(){
            this->data.clear();
        }
};
