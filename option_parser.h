 /*
 * Copyright (c) 2016 Dmytro Shestakov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <algorithm>
#include <vector>
#include <utility>
#include <iterator>
#include <exception>

namespace Opts {

    using std::cout;
    using std::cerr;
    using std::endl;
    using std::string;
    using std::vector;
    using std::pair;
    using std::tuple;
    using std::tie;
    using std::transform;
    using std::copy;
    using std::back_inserter;
    using std::exception;

    class Parser
    {
    private:
        vector<string> args_;
        size_t knownArgsEndPosition_{};
        template<typename... Strings>
        pair<int, size_t> Find_impl(int index, const char* str, Strings... strings)
        {
            for(size_t i = 0; i < args_.size(); ++i) {
                if(args_[i] == str) {
                    knownArgsEndPosition_ = knownArgsEndPosition_ < i + 1 ? i + 1 : knownArgsEndPosition_;
                    return{i, index};
                }
            }
            return Find_impl(index + 1, strings...);
        }
        pair<int, size_t> Find_impl(size_t)
        {
            return{-1, 0};
        }
    public:
        //Result = -1 : Not found
        template<typename... Strings>
        tuple<int, size_t, vector<string>> FindMultiple(int expectedArgsNumber, const char* str, Strings... strings)
        {
            auto& result1 = Find_impl(0, str, strings...);
            const auto& arg = args_[result1.first];
            pair<int, vector<string>> result2;
            if(result1.first >= 0) {
                if(expectedArgsNumber == 0) {
                    return{result1.first, result1.second, vector<string>{}};
                }
                else if(expectedArgsNumber == -1) {
                    result2 = FindUnsized(arg, result1.first);
                }
                else if(expectedArgsNumber > 0) {
                    result2 = Find(arg, 1, result1.first);
                }
            }
            return{result1.first, result1.second, result2.second};
        }
        template<typename... Strings>
        pair<int, size_t> FindMultiple(const char* str, Strings... strings)
        {
            return Find_impl(0, str, strings...);
        }
        pair<int, vector<string>> FindUnsized(const string& str, size_t startFrom = 0)
        {
            for(size_t i = startFrom; i < args_.size(); ++i) {
                if(args_[i] == str) {
                    vector<string> args;
                    auto argPosition = i;
                    ++i;
                    for(; i < args_.size(); ++i) {
                        if(args_[i][0] == '-') {
                            return{argPosition, args};
                        }
                        knownArgsEndPosition_ = knownArgsEndPosition_ < i + 1 ? i + 1 : knownArgsEndPosition_;
                        args.push_back(args_[i]);
                    }
                    return{argPosition, args};
                }
            }
            vector<string> emptyResult;
            return{-1, emptyResult};
        }
        pair<int, vector<string>> Find(const string& str, size_t expectedArgsNumber, size_t startFrom = 0)
        {
            for(size_t i = startFrom; i < args_.size(); ++i) {
                if(args_[i] == str) {
                    vector<string> args;
                    auto argPosition = i;
                    const auto maxExpected = i + expectedArgsNumber;
                    if(maxExpected >= args_.size()) {
                        cerr << "Option " << str << " must have " << expectedArgsNumber << " argument(s)" << endl;
                        return{argPosition, args};
                    }
                    ++i;
                    for(; i <= maxExpected; ++i) {
                        if(args_[i][0] == '-') {
                            cerr << "Option " << str << " must have " << expectedArgsNumber << " argument(s)" << endl;
                            return{argPosition, args};
                        }
                        knownArgsEndPosition_ = knownArgsEndPosition_ < i + 1 ? i + 1 : knownArgsEndPosition_;
                        args.push_back(args_[i]);
                    }
                    return{argPosition, args};
                }
            }
            vector<string> res;
            return{-1, res};
        }
        bool Find(const string& str)
        {
            int result;
            tie(result, std::ignore) = Find(str, 0);
            return result >= 0;
        }
        template<typename T = uint8_t>
        vector<T> ConvertToNumbers(vector<string>& data)
        {
            vector<T> result(data.size());
            transform(data.begin(), data.end(), result.begin(), [](string& str) {
                return static_cast<T>(stoi(str));
            });
            return result;
        }
        vector<string> GetTail()
        {
            vector<string> data;
            auto begin = args_.begin() + knownArgsEndPosition_;
            if(begin < args_.end()) {
                copy(begin, args_.end(), back_inserter(data));
            }
            return data;
        }
        Parser(int argc, const char* argv[]) : args_{static_cast<size_t>(argc - 1)}
        {
            if(argc == 1) {
                cout << "The program should take at least 1 parameter, -h for help";
                exit(1);
            }
            for(int i = 0; i + 1 < argc; ++i) {
                args_[i] = argv[i + 1];
                transform(args_[i].begin(), args_[i].end(), args_[i].begin(), ::tolower);
            }
        }
    };

    class ParsePortBaudrate
    {
    private:
        string port_{};
        int32_t baudRate_{};
    public:
        ParsePortBaudrate(Parser& parser)
        {
            if(parser.Find("-h")) {
                return;
            }
            int result;
            vector<string> keyValues;
            // Parse port name
            tie(result, keyValues) = parser.Find("-p", 1);
            if(result >= 0) {
                if(keyValues.size()) {
                    port_ = keyValues[0];
                }
                else {
                    exit(1);
                }
            }
            else {
                cerr << "User should provide Port option (-p)" << endl;
                exit(1);
            }
            //Parse baud rate
            tie(result, keyValues) = parser.Find("-b", 1);
            if(result >= 0) {
                try {
                    baudRate_ = stoi(keyValues[0]);
                }
                catch(exception& e) {
                    cerr << "Baudrate value is not valid. " << e.what() << endl;
                    exit(1);
                }
            }
            else {
                baudRate_ = 9600;
            }
        }
        void PrintDescription()
        {
            cout << "-p <port>\r\n"
                "    examples: -p COM1 -p com99\r\n"
                "-b <baud>\r\n"
                "    examples: -b 19200 -b 9600\r\n"
                "    default: 9600\r\n";
        }
        const string& GetPort() const {
            return port_;
        }
        int32_t GetBaudRate() const {
            return baudRate_;
        }
    };

}//Opts
