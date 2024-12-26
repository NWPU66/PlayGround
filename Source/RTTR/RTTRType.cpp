#include <cstdlib>
#include <iostream>

#include <rttr/type.h>
#include <rttr/type>
using namespace rttr;

void RetrieveRTTRTypeObjects()
{
    struct Base
    {
        RTTR_ENABLE()
    };
    struct Derived : Base
    {
        RTTR_ENABLE(Base)
    };

    class D {
        RTTR_ENABLE()
    };

    type my_int_type  = type::get<int>();
    type my_bool_type = type::get(true);
    if (my_int_type != my_bool_type)
    {
        std::cout << "int and bool are different types" << std::endl;
    }

    int        int_obj     = 23;
    const int* int_obj_ptr = &int_obj;
    if (type::get<int*>() != type::get(int_obj_ptr))
    {
        std::cout << "const int* and int* are different types" << std::endl;
    }

    Derived d;
    Base&   base = d;
    if (type::get<Base>() != type::get(base))
    {
        std::cout << "Base and Derived are different types" << std::endl;
    }
    Base* base_ptr = &d;
    if (type::get<Derived>() != type::get(base_ptr))
    {
        std::cout << "Derived and Base* are different types" << std::endl;
    }
    if (type::get<Base*>() == type::get(base_ptr))
    {
        std::cout << "Base* and Derived* are the same type" << std::endl;
    }

    // Any top level cv-qualifier of the given type T will be removed.
    D       d1;
    const D d2;
    if (type::get(d1) == type::get(d2))
    {
        std::cout << "const and non-const are the same type" << std::endl;
    }

    if (type::get_by_name("int") == type::get<int>())
    {
        std::cout << "type::get_by_name(\"int\") == type::get<int>()" << std::endl;
    }
}

void QueryInformationFromRTTRType()
{
    struct D
    {
        RTTR_ENABLE()
    };

    std::cout << type::get<D>().get_name() << std::endl;
    std::cout << type::get<D>().is_class() << std::endl;               // true
    std::cout << type::get<D>().is_pointer() << std::endl;             // false
    std::cout << type::get<D*>().is_pointer() << std::endl;            // true
    std::cout << type::get<D>().is_array() << std::endl;               // false
    std::cout << type::get<D[50]>().is_array() << std::endl;           // true
    std::cout << type::get<std::vector<D>>().is_array() << std::endl;  // true
    std::cout << type::get<D>().is_arithmetic() << std::endl;          // false
    std::cout << type::get<D>().is_enumeration() << std::endl;         // false

    struct Base
    {
        RTTR_ENABLE()
    };
    struct Derived : Base, D
    {
        RTTR_ENABLE(Base, D)
    };
    Derived d;

    auto base_list = type::get(d).get_base_classes();
    for (auto& t : base_list)
    {
        std::cout << t.get_name() << std::endl;
    }

    if (type::get(d).is_derived_from<Base>()) { std::cout << "Derived from Base" << std::endl; }
}

void RegisterClassHierarchy()
{
    struct Base
    {
        RTTR_ENABLE()
    };

    struct Other
    {
        RTTR_ENABLE()
    };

    struct Derived : Base
    {
        RTTR_ENABLE(Base)
    };

    struct MultipleDerived : Base, Other
    {
        RTTR_ENABLE(Base, Other)
    };

    for (auto& cls : type::get<MultipleDerived>().get_base_classes())
    {
        std::cout << cls.get_name() << std::endl;
    }
}

void RTTRCastVSDynamic_cast1()
{
    struct A
    {
        RTTR_ENABLE()
    };
    struct B : A
    {
        RTTR_ENABLE(A)
    };
    struct C : B
    {
        RTTR_ENABLE(B)
    };

    C  c;
    A* a = &c;
    B* b = rttr_cast<B*>(a);
}

void RTTRCastVSDynamic_cast2()
{
    struct A
    {
        RTTR_ENABLE()
    };
    struct B
    {
        RTTR_ENABLE()
    };
    struct C : A, B
    {
        RTTR_ENABLE(A, B)
    };

    C  c;
    A* a = &c;
    B* b = rttr_cast<B*>(a);
}

int main(int argc, char** argv)
{
    RetrieveRTTRTypeObjects();
    QueryInformationFromRTTRType();
    RegisterClassHierarchy();
    RTTRCastVSDynamic_cast1();
    RTTRCastVSDynamic_cast2();
    return EXIT_SUCCESS;
}
