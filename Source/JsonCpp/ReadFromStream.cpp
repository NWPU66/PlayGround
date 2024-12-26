#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "json/json.h"
#include "json/writer.h"

const std::string filePath = "E:/Study/CodeProj/PlayGround/Source/JsonCpp/withComment.json";

int main(int argc, char** argv)
{
    Json::Value   root;
    std::ifstream ifs(filePath);

    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!Json::parseFromStream(builder, ifs, &root, &errs))
    {
        std::cout << errs << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << root << std::endl;

    std::cout << root["key"].asString() << std::endl;

    ifs.close();
    return EXIT_SUCCESS;
}