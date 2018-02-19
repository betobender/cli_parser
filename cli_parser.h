/*
MIT License

Copyright (c) 2018 Roberto Bender

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <string>
#include <map>
#include <list>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <functional>
#include <exception>

#ifndef CLI_MAX_LINE_WIDTH
#define CLI_MAX_LINE_WIDTH 80
#endif

namespace cli
{
    class ParsingException : public std::exception
    {
    public:
        ParsingException(const std::string& message)
            : mMessage(message)
        {}

        const char* what() const throw() override
        {
            return mMessage.c_str();
        }
    private:
        std::string mMessage;
    };

    class Parser
    {
    public:
        enum ParsingResult
        {
            PARSED_OK = 0,
            PARSED_HELP = 1,
            PARSED_FAILED = -2,
            PARSED_FAILED_VALIDATOR = -3,
        };

        class Option
        {
        public:

            class Argument
            {
            public:
                Argument(const std::string& id, const std::string& desc)
                    : mId(id)
                    , mDesc(desc)
                {}

            private:
                std::string mId;
                std::string mDesc;
                std::string mValue;

                friend class Parser;
            };

            Option(
                const std::list<std::string>& opts,
                const std::string& description = "",
                bool mandatory = true, 
                const std::vector<Argument>& args = {},
                std::function<bool(Option&)> validator = nullptr
            )
                : mOpts(opts)
                , mDescription(description)
                , mMandatory(mandatory)
                , mArgsRef(args)
                , mProvided(false)
                , mValidator(validator)
            {
                for(size_t i = 0; i < mArgsRef.size(); ++i)
                    mArgsMap.insert(std::make_pair(mArgsRef[i].mId, i));
            }

            const std::string& value(const std::string& id) const
            {
                if (mArgsMap.find(id) == mArgsMap.end())
                    throw ParsingException("Invalid Argument");
                return mArgsRef[mArgsMap.at(id)].mValue;
            }

        private:
            std::list<std::string> mOpts;
            std::string mDescription;
            bool mMandatory;
            std::vector<Argument> mArgsRef;
            std::map<std::string, size_t> mArgsMap;
            bool mProvided;
            std::function<bool(Option&)> mValidator;

            friend class Parser;

            bool checkMandatory()
            {
                return !mMandatory || mProvided;
            }
        };
        
        Parser(const std::string& program = "", const std::string& version = "", const std::string& description = "")
            : mProgram(program)
            , mVersion(version)
            , mDescription(description)
        {
        }

        void addOptions(const std::list<Option>& options)
        {
            for (auto option : options)
                addOption(option);
        }

        void addOption(Option& option)
        {
            mOptionRefs.push_back(option);
            Option& ref = mOptionRefs.back();
            for (auto opt : ref.mOpts)
                mOptionsMap[opt] = &ref;
        }

        std::string composeHelpString() const
        {
            std::stringstream ss;
            
            if (mProgram.length() > 0)
            {
                ss << separtor() << std::endl;
                ss << std::left << std::setw(CLI_MAX_LINE_WIDTH * 75 / 100) << mProgram;
                ss << std::right << std::setw(CLI_MAX_LINE_WIDTH * 25 / 100) << mVersion;
                ss << std::endl << separtor() << std::endl;
            }
            
            if(mDescription.length() > 0)
                ss << splitWords(mDescription) << std::endl << separtor() << std::endl;

            ss << std::endl;

            for (auto opt : mOptionRefs)
            {
                std::string optsStr = ((opt.mMandatory) ? "*" : "") + opt.mOpts.front();
                for (auto it = ++opt.mOpts.begin(); it != opt.mOpts.end(); ++it)
                    optsStr += ", " + *it;

                if (opt.mArgsRef.size() > 0)
                    optsStr += " {args...}";

                ss << std::left << std::setw(CLI_MAX_LINE_WIDTH * 30 / 100) << optsStr;
                ss << std::left << splitWords(opt.mDescription, CLI_MAX_LINE_WIDTH * 70 / 100, std::string(CLI_MAX_LINE_WIDTH * 30 / 100, ' '));
                ss << std::endl;

                if (opt.mArgsRef.size() > 0)
                {
                    ss << std::string(CLI_MAX_LINE_WIDTH * 30 / 100, ' ') << "Arguments: " << std::endl;

                    for (auto arg : opt.mArgsRef)
                    {
                        std::string argStr = "{" + arg.mId + "} => ";
                        ss << std::string(CLI_MAX_LINE_WIDTH * 30 / 100, ' ') << argStr;
                        ss << splitWords(arg.mDesc, CLI_MAX_LINE_WIDTH * 70 / 100, std::string(CLI_MAX_LINE_WIDTH * 30 / 100, ' '));
                        ss << std::endl;
                    }
                }
            }

            return ss.str();
        }

        std::string separtor() const
        {
            return std::string(CLI_MAX_LINE_WIDTH, '-');
        }

        std::string splitWords(const std::string& value, size_t width = CLI_MAX_LINE_WIDTH, const std::string& padStr = "") const
        {
            std::stringstream ss;
            std::string copy = value;
            std::string padStrInUse;

            while (copy.size() > width)
            {
                std::string block = copy.substr(0, width);
                size_t index = block.find_last_of(' ');
                
                if (index != std::string::npos)
                {
                    ss << padStrInUse << block.substr(0, index) << std::endl;
                    copy = copy.substr(index + 1);
                }
                else
                {
                    ss << padStrInUse << block << std::endl;
                    copy = copy.substr(width);
                }

                padStrInUse = padStr;
            }
            
            ss << padStrInUse << copy;
            return ss.str();
        }

        ParsingResult parse(int argc, char* argv[])
        {
            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];
                if (arg == "--help" || arg == "-h" || arg == "/?")
                {
                    std::cout << composeHelpString() << std::endl;
                    return PARSED_HELP;
                }

                if (mOptionsMap.find(arg) == mOptionsMap.end())
                {
                    std::cerr << "Invalid argument {'" << arg << "'}. Please use --help for more information." << std::endl;
                    return PARSED_FAILED;
                }

                auto& opt = mOptionsMap[arg];
                
                for(auto& innerArg : opt->mArgsRef)
                {
                    if (i + 1 >= argc)
                    {
                        std::cerr << "Missing argument {'" << innerArg.mId << "'} for parameter '" << arg << "'. Please use --help for more information." << std::endl;
                        return PARSED_FAILED;
                    }
                    std::string subArg = argv[++i];
                    innerArg.mValue = subArg;
                }

                if (opt->mValidator != nullptr && !opt->mValidator(*opt))
                    return PARSED_FAILED_VALIDATOR;

                opt->mProvided = true;
            }

            for (auto& opt : mOptionRefs)
            {
                if (!opt.checkMandatory())
                {
                    std::cerr << "Mandatory parameter {'" << opt.mOpts.front() << "'} not provided. Please use --help for more information." << std::endl;
                    return PARSED_FAILED;
                }
            }

            return PARSED_OK;
        }

        const Option& operator () (const std::string& opt) const
        {
            if (mOptionsMap.find(opt) == mOptionsMap.end())
                throw ParsingException("Option Not Found!");
            return *mOptionsMap.at(opt);
        }

    private:
        std::string mProgram;
        std::string mVersion;
        std::string mDescription;
        std::list<Option> mOptionRefs;
        std::map<std::string, Option*> mOptionsMap;
    };
}

#endif
