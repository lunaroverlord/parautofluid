/*
 * Logger class (from another personal project)
 */
#include <sstream>
#include <fstream>

class Log
{
public:
        Log(string outputFile) : os(""), outputFile(outputFile) {}
        Log(const Log&) {}
        ~Log()
        {
                Commit();
        }

        template <typename T>
        Log& operator<<(const T& rex)
        {
                os<<rex;
                return *this;
        }
        Log& operator<<(ostream& (*rax) (ostream&))
        {
                os<<rax; //endl
                //Commit();
                return *this;
        }

        ostringstream& operator()()
        {
                return os;
        }

        void Commit()
        {
                ofstream out(outputFile);
                out<<os.str();
                out.close();
        }

private:
	string outputFile;
        ostringstream os;
};
