#include <cmath>
#include <cstdlib>
#include <iostream>

#include <rttr/registration>
#include <rttr/variant.h>
using namespace rttr;

static void f()
{
    std::cout << "Hello World" << std::endl;
}

void HelloWorld()
{
    type::invoke("f0", {});
}

void Method()
{
    // option 1
    variant return_value = type::invoke("pow", {9.0, 2.0});
    if (return_value.is_valid() && return_value.is_type<double>())
    {
        std::cout << return_value.get_value<double>() << std::endl;
    }

    // option 2
    method meth = type::get_global_method("pow");
    if (meth)
    {
        return_value = meth.invoke({}, 9.0, 3.0);
        if (return_value.is_valid() && return_value.is_type<double>())
        {
            std::cout << return_value.get_value<double>() << std::endl;
        }
    }
}

static const double pi          = 3.14259;
static std::string  global_text = "abc";

void set_text(const std::string& text)
{
    global_text = text;
}
const std::string& get_text()
{
    return global_text;
}

void Property()
{
    // option 1
    variant value = type::get_property_value("PI");
    if (value && value.is_type<double>())
    {
        std::cout << value.get_value<double>() << std::endl;  // outputs: "3.14259"
    }

    // option 2
    property prop = type::get_global_property("PI");
    if (prop)
    {
        value = prop.get_value({});
        if (value.is_valid() && value.is_type<double>())
        {
            std::cout << value.get_value<double>() << std::endl;  // outputs: "3.14259"
        }
    }

    property prop1 = type::get_global_property("global_text");
    std::cout << prop1.get_value({}).to_string() << std::endl;
    std::cout << "success?: " << prop1.set_value({}, "Hello, World!") << std::endl;
    std::cout << prop1.get_value({}).to_string() << std::endl;
    // FIXME - prop1.set_value({}, "Hello, World!") 设置失败

    // std::cout << type::get_property_value("global_text").to_string() << std::endl;
    // type::set_property_value("global_text", "Hello, World!");
    // std::cout << type::get_property_value("global_text").to_string() << std::endl;
}

enum class E_Alignment {
    AlignLeft    = 0x0001,
    AlignRight   = 0x0002,
    AlignHCenter = 0x0004,
    AlignJustify = 0x0008,
};

void Enum()
{
    type enum_type = type::get_by_name("E_Alignment");
    if (enum_type && enum_type.is_enumeration())
    {
        enumeration enum_align = enum_type.get_enumeration();
        std::string name       = enum_align.value_to_name(E_Alignment::AlignHCenter).to_string();
        std::cout << name << std::endl;  // prints "AlignHCenter"
        variant     var   = enum_align.name_to_value(name);
        E_Alignment value = var.get_value<E_Alignment>();  // stores value 'AlignHCenter'
    }
}

void Variant()
{
    variant var;
    var   = 23;            // copy integer
    int x = var.to_int();  // x = 23

    var   = "Hello World";
    int y = var.to_int();

    var = "42";                              // contains a std::string
    std::cout << var.to_int() << std::endl;  // convert std::string to integer and prints "42"

    int my_array[100];
    var       = (int*)my_array;             // copies the content of my_array into var
    auto& arr = var.get_value<int[100]>();  // extracts the content of var by reference
}

struct test_class
{
    test_class(int value) : m_value(value) {}
    static test_class* Create() { return nullptr; }
    void               print_value() const { std::cout << m_value << std::endl; }

    int  m_value;
    void set_value(int x) { m_value = x; }
    int  get_value() const { return m_value; }

    void f() {}
    void f(int) {}
    void f(int) const {}

    RTTR_ENABLE()
};

void Class()
{
    {
        // option 1
        type class_type = type::get_by_name("test_class");
        if (class_type)
        {
            variant obj = class_type.create({23});  // constructed are created on the heap
            std::cout << obj.get_type().get_name() << std::endl;
            if (obj.get_type().is_pointer()) { class_type.destroy(obj); }
        }

        // option 2
        if (class_type)
        {
            constructor ctor = class_type.get_constructor({type::get<int>()});
            variant     obj  = ctor.invoke(23);
            if (obj.get_type().is_pointer())
            {
                destructor dtor = class_type.get_destructor();
                dtor.invoke(obj);
            }
        }
    }

    {
        test_class obj(42);
        type       class_type = type::get_by_name("test_class");

        // option 1
        class_type.invoke("print_value", obj, {});

        // option 2
        method print_meth = class_type.get_method("print_value");
        print_meth.invoke(obj);  // prints "42"
    }

    {
        std::shared_ptr<test_class> obj = std::make_shared<test_class>(23);
        method meth                     = type::get_by_name("test_class").get_method("print_value");
        meth.invoke(obj);         // successful invoke
        meth.invoke(*obj.get());  // successful invoke
        variant var = obj;
        std::cout << var.get_type().get_name() << std::endl;
        meth.invoke(var);  // successful invoke
    }

    {
        test_class obj(0);
        type       class_type = type::get_by_name("test_class");
        // option 1
        bool success = class_type.set_property_value("value", obj, 50);
        std::cout << obj.m_value << std::endl;  // prints "50"
        // option 2
        property prop = class_type.get_property("value");
        success       = prop.set_value(obj, 24);
        std::cout << obj.m_value << std::endl;  // prints "24"
    }
}

enum class MetaData_Type { SCRIPTABLE, GUI };
MetaData_Type g_Value;

void MetaData()
{
    property prop  = type::get_global_property("value");
    variant  value = prop.get_metadata(MetaData_Type::SCRIPTABLE);
    std::cout << value.get_value<bool>() << std::endl;  // prints "false"

    value = prop.get_metadata("Description");
    std::cout << value.get_value<std::string>() << std::endl;  // prints "This is a value."
}

RTTR_REGISTRATION
{
    // method
    registration::method("f0", &f);
    registration::method("atoi", &atoi);
    registration::method("sin", select_overload<float(float)>(&sin))
        .method("sin", select_overload<double(double)>(&sin));  // overload
    registration::method("pow", select_overload<double(double, double)>(&pow));

    // property
    registration::property_readonly("PI", &pi);
    registration::property("global_text", &get_text, &set_text);

    // Enum
    registration::enumeration<E_Alignment>("E_Alignment")(
        value("AlignLeft", E_Alignment::AlignLeft),        //
        value("AlignRight", E_Alignment::AlignRight),      //
        value("AlignHCenter", E_Alignment::AlignHCenter),  //
        value("AlignJustify", E_Alignment::AlignJustify)   //
    );

    // Class
    registration::class_<test_class>("test_class")
        .constructor<int>()
        .constructor(&test_class::Create)
        .method("print_value", &test_class::print_value)
        .property("value", &test_class::get_value, &test_class::set_value)
        .method("f", select_overload<void(void)>(&test_class::f))
        .method("f", select_overload<void(int)>(&test_class::f))
        .method("f", select_const(&test_class::f));

    // MetaData
    registration::property("value", &g_Value)(       //
        metadata(MetaData_Type::SCRIPTABLE, false),  //
        metadata("Description", "This is a value.")  //
    );
}

int main()
{
    HelloWorld();
    Method();
    Property();
    Enum();
    Variant();
    Class();
    MetaData();
    return EXIT_SUCCESS;
}