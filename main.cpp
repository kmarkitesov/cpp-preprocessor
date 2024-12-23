#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

static const regex local_include("\\s*#\\s*include\\s*\"([^\"]*)\"\\s*");
static const regex system_include("\\s*#\\s*include\\s*<([^>]*)>\\s*");

bool ProcessIncludeFile(const string& include_path, const path& current_file, ostream& output_stream, const vector<path>& include_directories, size_t line_number);
bool Preprocess(istream& input_stream, ostream& output_stream, const path& current_file, const vector<path>& include_directories);
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool ProcessIncludeFile(const string& include_path, const path& current_file, ostream& output_stream, const vector<path>& include_directories, size_t line_number) {
    path include_file = current_file.parent_path() / include_path;
    ifstream include_stream(include_file);

    for (const auto& directory : include_directories) {
        if (include_stream.is_open()) break;
        include_file = directory / include_path;
        include_stream.open(include_file);
    }

    if (!include_stream.is_open()) {
        cout << "unknown include file " << include_path << " at file " << current_file.string() << " at line " << line_number << endl;
        return false;
    }

    return Preprocess(include_stream, output_stream, include_file, include_directories);
}

bool Preprocess(istream& input_stream, ostream& output_stream, const path& current_file, const vector<path>& include_directories){
    string file_line;
    size_t file_line_number = 0;

    while(getline(input_stream, file_line)){
        ++file_line_number;
        smatch match;

        if(regex_match(file_line, match, local_include)){
           if(!ProcessIncludeFile(match[1].str(), current_file, output_stream, include_directories, file_line_number)){
                return false;
           }
        } 
        else if (regex_match(file_line, match, system_include)){
            if(!ProcessIncludeFile(match[1].str(), current_file, output_stream, include_directories, file_line_number)){
                return false;
           }
        }
        else {
            output_stream << file_line << '\n';
        }
    }

    return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    ifstream istream(in_file);
    if(!istream.is_open()){
        return false;
    }

    ofstream ostream(out_file);
    if(!ostream.is_open()){
        return false;
    }

    return Preprocess(istream, ostream, in_file, include_directories);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}