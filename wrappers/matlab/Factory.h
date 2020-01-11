#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <functional>
#include <map>
#include <string>
#include <mex.h>
#include <matrix.h>

typedef void mxFunc(int, mxArray*[], int, const mxArray*[]);

struct func_data {
    std::function<mxFunc> f;
    int out, in_min, in_max;
    func_data() : f(), out(0), in_min(0), in_max(0) {};
    func_data(std::function<mxFunc> function, int out_args, int in_args) : func_data(function, out_args, in_args, in_args) {}
    func_data(std::function<mxFunc> function, int out_args, int in_min_args, int in_max_args) : f(function), out(out_args), in_min(in_min_args), in_max(in_max_args) {}
};

class ClassFactory
{
private:
    std::string name;
    std::map<std::string, func_data> funcs;
public:
    ClassFactory(std::string n) : name(n), funcs() {}
    void record(std::string fname, int out, int in, std::function<mxFunc> func)
    {
        funcs.emplace(fname, func_data(func, out, in));
    }
    void record(std::string fname, int out, int in_min, int in_max, std::function<mxFunc> func)
    {
        funcs.emplace(fname, func_data(func, out, in_min, in_max));
    }

    std::string get_name() { return name; }

    func_data get(std::string f){
        auto func = funcs.find(f);
        if (func == funcs.end()) return func_data();
        return func->second;
    }
};

class Factory
{
private:
    std::map<std::string, ClassFactory> classes;
public:
    Factory() = default;
    void record(ClassFactory cls) { classes.emplace(cls.get_name(), cls); }

    func_data get(std::string c, std::string f){
        auto cls = classes.find(c);
        if (cls == classes.end()) return func_data();
        return cls->second.get(f);
    }
};

#endif