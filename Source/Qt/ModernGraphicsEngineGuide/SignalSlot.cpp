#include <cstdlib>

#include <QApplication>
#include <QtWidgets>
#include <qapplication.h>
#include <qwindow.h>
#include <signal.h>

/*
class Student{
    Signal nameChanged(string OldName, string NewName);
public:
    void setName(string inName){
        nameChanged.Emit(mName，inName);
        mName = inName;
    }
private:
    string mName;
};

class Tearcher{
public:
    void OnStudentNameChanged(string OldName, string NewName){
        //do something
    }
};

class Class{
protected:
    void connectEveryone(){
         for(auto Student:mStudents){
             Student.nameChanged.Connect(mTeacher,&Tearcher::OnStudentNameChanged);
         }
    }
private:
    Teacher mTeacher;
    TArray<Student> mStudents;
};
*/

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    Q_ASSERT(QApplication::instance() == &app);

    QWidget widget;
    widget.setWindowTitle("Hello QWigdet!");
    widget.show();

    return QApplication::exec();
}