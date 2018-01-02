#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <functional>
#include <map>
#include <mex.h>
#include <matrix.h>

typedef void mxFunc(int, mxArray*[], int, const mxArray*[]);

class ClassFactory
{
private:
    std::map<std::string, std::function<mxFunc>> funcs;
public:
    ClassFactory() = default;
    void record(std::string name, std::function<mxFunc> func) { funcs.emplace(name, func); }

    std::function<mxFunc> get(std::string f){
        auto func = funcs.find(f);
        if (func == funcs.end()) return std::function<mxFunc>();
        return func->second;
    }
};

class Factory
{
private:
    std::map<std::string, ClassFactory> classes;
public:
    Factory() = default;
    void record(std::string name, ClassFactory cls) { classes.emplace(name, cls); }

    std::function<mxFunc> get(std::string c, std::string f){
        auto cls = classes.find(c);
        if (cls == classes.end()) return std::function<mxFunc>();
        return cls->second.get(f);
    }
};

#endif