#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),
_first_unread(0),_eof(false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // receive data that has already assembled
    if(data.empty()||data.size()+index-1<_first_unread){
        return;
    }
    int first_unacceptable = _first_unread + _capacity ;
    if(index>=first_unacceptable){
        //? what if eof?
        return;
    }
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
}

void StreamReassembler::push2list(const string &data, const size_t index){
    Data new_data(index,data);
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
            return;
        }

    }

}
void StreamReassembler::write2stream(){
    for(auto iter = _data_list.begin();iter!=_data_list.end()&&_output.remaining_capacity()>0;){
        auto &_index = iter->_start_index;
        auto &_data = iter->_data;
        if(_index == _first_unread){
            auto bytes = _output.write(_data);
            if(bytes=_data.size()){
                _index += bytes;
                _data = _data.substr(bytes);
            }else{
                iter = _data_list.erase(iter);
            }
            _first_unread += bytes;
        }
    }
}