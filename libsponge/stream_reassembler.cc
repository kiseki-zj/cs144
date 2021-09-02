#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _nAssembled(0), _nUnassembled(0), _eof(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //data非法，直接return的情况
    if (index + data.size() <= _nAssembled) {
        if (eof) _output.end_input();
        return ;
    }
    if (index >= _nAssembled - _output.buffer_size() + _capacity) {
        return ;
    }
    if (data.empty()) {
        if (eof) _output.end_input();
        return ;
    }
    //data合法，取可用区间
    size_t _firstIndex = _nAssembled - _output.buffer_size();
    size_t resIndex = index;
    if (index < _nAssembled) {
        resIndex = _nAssembled;
    }
    string resData = data.substr(resIndex - index, _firstIndex + _capacity - resIndex);//substr长度自动截断
    if (eof && resIndex + resData.size() == index + data.size())
        _eof = 1;
    merge_substr(resIndex, resData);//把resData合并到set里
    /*
    cout << "after push:" << endl;
    for (auto it = assembler.begin(); it != assembler.end(); it++) {
        cout << "[ " << it->index << ", " << it->index + it->data.size() -1 << "]" << endl; 
    }
    cout << "------------------------------" << endl;
    */
    //在set里找第一个substr， 看能否write
    set<substr_t>::iterator it = assembler.begin();
    if (it->index == _nAssembled) { //可以write
    /*
        cout << "---------write------------" << endl;
        cout << "write " << it->data.size() << "bytes" << endl;
        cout << "has writen:[ 0 , " << _nAssembled + it->data.size() -1 << "]" << endl;
        cout << "--------------------------" << endl;
    */
        size_t bytes_writen = _output.write(it->data);
        _nUnassembled -= it->data.size(); 
        assembler.erase(it);
        _nAssembled += bytes_writen;
    }
    if (_eof && _nUnassembled == 0)
        _output.end_input();
    return ;
}

void StreamReassembler::merge_substr(size_t index, string& data) {
    substr_t substr(index, data);
    if (assembler.empty()) {
        assembler.insert(substr);
        _nUnassembled += substr.data.size();
        return ;
    }
    set<substr_t>::iterator it = assembler.lower_bound(substr);
    if (it == assembler.end()) {
        set<substr_t>::iterator f_it = --it;
        if (index + data.size() <= f_it->index + f_it->data.size()) {
            return ;
        }
        if (index > f_it->index + f_it->data.size()) {
            assembler.insert(substr);
            _nUnassembled += substr.data.size();
        }
        else {
            size_t len = f_it->data.size();
            string _ = f_it->data + data.substr(f_it->index + f_it->data.size() - index);
            _nUnassembled += _.size() - len;
            size_t _index = f_it->index;
            assembler.erase(f_it);
            assembler.insert(substr_t(_index, _));
        }
    }
    else if (it == assembler.begin()) {
        if (index + data.size() < it->index) {
            assembler.insert(substr_t(index, data));
            _nUnassembled += data.size();
            return ;
        }
        int flag = 0;
        while (it != assembler.end()) {
            if (index + data.size() > it->index + it->data.size()) {
                _nUnassembled -= it->data.size();
                assembler.erase(it++);
            }
            else {
                flag = 1;
                if (index + data.size() < it->index) {
                    assembler.insert(substr_t(index, data));
                    _nUnassembled += data.size();
                    break;
                }
                else {
                    string _data = data + it->data.substr(index + data.size() - it->index);
                    _nUnassembled -= it->data.size();
                    assembler.erase(it);
                    assembler.insert(substr_t(index, _data));
                    _nUnassembled += _data.size();
                    break;
                }
            }
        }
        if (!flag) {
            assembler.insert(substr_t(index, data));
            _nUnassembled += data.size();
        }
    }
    else {
        int flag = 0;
        while (it != assembler.end()) {
            if (index + data.size() > it->index + it->data.size()) {
                _nUnassembled -= it->data.size();
                assembler.erase(it++);
            }
            else {
                flag = 1;
                if (index + data.size() < it->index) {
                    assembler.insert(substr_t(index, data));
                    _nUnassembled += data.size();
                    break;
                }
                else {
                    string _data = data + it->data.substr(index + data.size() - it->index);
                    _nUnassembled -= it->data.size();
                    assembler.erase(it);
                    assembler.insert(substr_t(index, _data));
                    _nUnassembled += _data.size();
                    break;
                }
            }
        }
        if (!flag) {
            assembler.insert(substr_t(index, data));
            _nUnassembled += data.size();
        }
        it = assembler.lower_bound(substr_t(index, ""));
        set<substr_t>::iterator f_it = it;
        f_it--;
        if (f_it->index + f_it->data.size() < index) {
            return ;
        }
        else {
            if (f_it->index + f_it->data.size() < it->index + it->data.size()) {
                string _data = f_it->data.substr(0, index - f_it->index) + it->data;
                substr_t _(f_it->index, _data);
                _nUnassembled -= f_it->data.size();
                assembler.erase(f_it);
                _nUnassembled -= it->data.size();
                assembler.erase(it);
                _nUnassembled += _data.size();
                assembler.insert(_);
            }
            else {
                _nUnassembled -= it->data.size();
                assembler.erase(it);
            }
        }
    }
}

size_t StreamReassembler::unassembled_bytes() const { return {_nUnassembled}; }

size_t StreamReassembler::firstunassembled() const { return _nAssembled; }

bool StreamReassembler::empty() const { return {_nUnassembled == 0}; }
