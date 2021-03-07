#include "stream_reassembler.hh"
#include <iostream>
#include <algorithm>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),_unassembled_bytes(0),
_first_unread(0),_eof(false),_last_acceptable(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof){
        _eof=1;
        _last_acceptable = index +data.size();
    }
    // receive data that has already assembled
    if(data.empty()||data.size()+index<_first_unread+1){
        write2stream();
        return;
    }
    size_t first_unacceptable = _first_unread + _capacity ;
    if(index>=first_unacceptable){
        write2stream();
        //? what if eof?
        return;
    }

    push2list(data,index);
    write2stream();
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _unassembled_bytes;
 }


//! \brief Is the internal state empty (other than the output stream)?
//! \returns `true` if no substrings are waiting to be assembled
bool StreamReassembler::empty() const { 
    return _data_list.empty();
 }

// the data1 is the new data
int StreamReassembler::type_overlap(Data data1,Data data2){
    uint64_t start1 = data1._start_index;
    uint64_t start2 = data2._start_index;
    uint64_t end1 = data1._start_index + data1._data.size()-1;
    uint64_t end2 = data2._start_index + data2._data.size()-1;
    //no overlaping
    if(end2<start1||start2>end1){
        return 0;
    }

    //       [    ]
    //    [     ]
    // the new data contains the right part of the old
    if(start1>start2&&end1>end2){
        return 1;
    }

    //     [  ]
    //   [     ]
    // the new data is contained in the old
    if(start1>start2&&end1<=end2){
        return 2;
    }

    //     [   ]
    //     [ ]
    //the new data constans the old
    if(start1<=start2&&end1>end2){
        return 3;
    }

    //     [   ]
    //     [    ]
    // the new data contains the left part of the old
    if(start1<=start2&&end1<=end2){
        return 4;
    }
    return 5;
}

void StreamReassembler::push2list(const string &data, const size_t index){
    string str = data;
    size_t i = index;
    if(index < _first_unread){
        size_t len = _first_unread - index;
        str=str.substr(len);
        i = _first_unread;
    }
    Data new_data(i,str);
    for(auto iter = _data_list.begin();iter!=_data_list.end();){
        int type = type_overlap(new_data,*iter);
        if(!type){
            iter++;
        }else if(type == 1){
            auto &_index = iter->_start_index;
            auto &_data = iter->_data;
            size_t len = _index + _data.size() - index;
            _unassembled_bytes -= len;
            _data = _data.substr(0,_data.size()-len);
            iter++;
        }else if(type == 2){
            auto &_index = iter->_start_index;
            auto &_data = iter->_data;
            _data.replace(_data.begin()+index - _index,_data.begin()+index - _index+data.size(),data.begin(),data.end());
            return;
        }else if(type == 3){
            _unassembled_bytes -= iter->_data.size();
            iter = _data_list.erase(iter);
        }else if (type == 4){
            auto &_index = iter->_start_index;
            auto &_data = iter->_data;
            size_t len = index + data.size() - _index;
            _unassembled_bytes -= len;
            _data = _data.substr(len);
            _index += len;
        }

    }
    _data_list.push_back(new_data);
    _unassembled_bytes += new_data._data.size();

}
void StreamReassembler::write2stream(){

    std::sort(_data_list.begin(),_data_list.end(),[](Data&d1,Data&d2){return d1._start_index<d2._start_index;});
    for(auto iter = _data_list.begin();iter!=_data_list.end()&&_output.remaining_capacity()>0;){
        auto &_index = iter->_start_index;
        auto &_data = iter->_data;
        if(_index == _first_unread){
            auto bytes = _output.write(_data);
            if(bytes != _data.size()){
                _index += bytes;
                _data = _data.substr(bytes);
            }else{
                iter = _data_list.erase(iter);
            }
            _unassembled_bytes -= bytes;
            _first_unread += bytes;
        }else{
            iter++;
        }
    }
    if(_eof&&_last_acceptable == _first_unread&&_data_list.empty()){
        _output.end_input();
    }

}
