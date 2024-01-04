#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>
#include "common_types.h"
#include "crash.h"

struct Rejector {
    u16 mask;
    u16 unexpected;
    bool Rejects(u16 instruction) const {
        return (instruction & mask) == unexpected;
    }
};

extern std::unordered_map<std::string, u32> call_count;

template <typename Visitor>
class Matcher {
public:
    using visitor_type = Visitor;
    using handler_return_type = typename Visitor::instruction_return_type;
    using handler_function = std::function<handler_return_type(Visitor&, u16, u16)>;

    Matcher(const char* const name, u16 mask, u16 expected, bool expanded, handler_function func)
        : name{name}, mask{mask}, expected{expected}, expanded{expanded}, fn{std::move(func)} {
        std::stringstream stream;
        stream << name << " 0x" << std::hex << expected;
        identifier = stream.str();
    }

    static Matcher AllMatcher(handler_function func) {
        return Matcher("*", 0, 0, false, std::move(func));
    }

    const char* GetName() const {
        return name;
    }

    bool NeedExpansion() const {
        return expanded;
    }

    bool Matches(u16 instruction) const {
        return (instruction & mask) == expected &&
               std::none_of(rejectors.begin(), rejectors.end(),
                            [instruction](const Rejector& rejector) {
                                return rejector.Rejects(instruction);
                            });
    }

    Matcher Except(Rejector rejector) const {
        Matcher new_matcher(*this);
        new_matcher.rejectors.push_back(rejector);
        return new_matcher;
    }

    handler_return_type call(Visitor& v, u16 instruction, u16 instruction_expansion = 0) const {
        ASSERT(Matches(instruction));
        call_count[identifier]++;
        return fn(v, instruction, instruction_expansion);
    }

private:
    const char* name;
    u16 mask;
    u16 expected;
    bool expanded;
    std::string identifier;
    handler_function fn;
    std::vector<Rejector> rejectors;
};
