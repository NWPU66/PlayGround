#include <cstdlib>
#include <iostream>

#include <rttr/registration>
using namespace rttr;

struct MyStruct
{
    MyStruct() {};
    void func(double) {};
    int  data;
};

RTTR_REGISTRATION
{
    registration::class_<MyStruct>("MyStruct")
        .constructor<>()
        .property("data", &MyStruct::data)
        .method("func", &MyStruct::func);
}

int main(int argc, char** argv)
{
    // Iterate over members
    {
        type t = type::get<MyStruct>();
        for (auto& prop : t.get_properties())
            std::cout << "name: " << prop.get_name() << std::endl;

        for (auto& meth : t.get_methods())
            std::cout << "name: " << meth.get_name() << std::endl;
    }

    // Constructing types
    {
        type    t   = type::get_by_name("MyStruct");
        variant var = t.create();  // will invoke the previously registered ctor

        constructor ctor = t.get_constructor();  // 2nd way with the constructor class
        var              = ctor.invoke();
        std::cout << var.get_type().get_name();  // prints 'MyStruct'
    }

    // Set/get properties
    {
        MyStruct obj;

        property prop = type::get(obj).get_property("data");
        prop.set_value(obj, 23);

        variant var_prop = prop.get_value(obj);
        std::cout << var_prop.to_int();  // prints '23'
    }

    // Invoke methods
    {
        MyStruct obj;

        method meth = type::get(obj).get_method("func");
        meth.invoke(obj, 42.0);

        variant var = type::get(obj).create();
        meth.invoke(var, 42.0);
    }

    return EXIT_SUCCESS;
}
