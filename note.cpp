#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>

#include "note.hpp"

using namespace std;

const tm day_start = { 0, 0, 0 };
const tm day_end = { 59, 59, 23 };

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool parse_ymd(const string str, tm *ymd)
{
    // 0000-00-00

    bool ok = str.length() == 10 &&
    is_digit(str.at(0)) &&
    is_digit(str.at(1)) &&
    is_digit(str.at(2)) &&
    is_digit(str.at(3)) &&
    str.at(4) == '-' &&
    is_digit(str.at(5)) &&
    is_digit(str.at(6)) &&
    str.at(7) == '-' &&
    is_digit(str.at(8)) &&
    is_digit(str.at(9));

    istringstream iss(str);
    char c;
    iss >> ymd->tm_year >> c >> ymd->tm_mon >> c >> ymd->tm_mday;
    ymd->tm_year -= 1900;
    ymd->tm_mon -= 1;

    return ok;
}

bool parse_md(const string str, tm *ymd)
{
    // 00-00

    bool ok = str.length() == 5 &&
    is_digit(str.at(0)) &&
    is_digit(str.at(1)) &&
    str.at(2) == '-' &&
    is_digit(str.at(3)) &&
    is_digit(str.at(4));

    istringstream iss(str);
    char c;
    iss >> ymd->tm_mon >> c >> ymd->tm_mday;
    ymd->tm_year = 0;
    ymd->tm_mon -= 1;

    return ok;
}

bool parse_hm(const string str, tm *ymd)
{
    // 00:00

    bool ok = str.length() == 5 &&
    is_digit(str.at(0)) &&
    is_digit(str.at(1)) &&
    str.at(2) == ':' &&
    is_digit(str.at(3)) &&
    is_digit(str.at(4));

    istringstream iss(str);
    char c;
    iss >> ymd->tm_hour >> c >> ymd->tm_min;
    ymd->tm_sec = 0;

    return ok;
}

void copy_hms(const tm *from, tm *to)
{
    to->tm_hour = from->tm_hour;
    to->tm_min = from->tm_min;
    to->tm_sec = from->tm_sec;
}

string hm_to_string(tm *the_tm)
{
    ostringstream oss;
    oss << setfill('0') << setw(2) << the_tm->tm_hour << ":" <<
    setfill('0') << setw(2) << the_tm->tm_min;

    return oss.str();
}

string md_to_string(tm *the_tm)
{
    ostringstream oss;
    oss << setfill('0') << setw(2) << (the_tm->tm_mon + 1) << "-" <<
    setfill('0') << setw(2) << the_tm->tm_mday;

    return oss.str();
}

string ymd_to_string(tm *the_tm)
{
    ostringstream oss;
    oss << setfill('0') << setw(4) << (the_tm->tm_year + 1900) << "-" <<
        setfill('0') << setw(2) << (the_tm->tm_mon + 1) << "-" <<
        setfill('0') << setw(2) << the_tm->tm_mday;

    return oss.str();
}

void print_note(Note note)
{
    switch (note.note_type) {
        case Note::DATE:
            cout << "DATE " << md_to_string(&note.start) << " "  <<
            md_to_string(&note.end) << " "  << hm_to_string(&note.start) << " " <<
            hm_to_string(&note.end);
            break;

        case Note::ONE_TIME:
            cout << "ONE_TIME " << ymd_to_string(&note.start) << " "  <<
                ymd_to_string(&note.end) << " "  << hm_to_string(&note.start) << " " <<
                hm_to_string(&note.end);
            break;

        case Note::DAY_OF_WEEK:
            cout << "DAY_OF_WEEK ";
            switch (note.day_of_week) {
                case 0:     cout << "SUN";      break;
                case 1:     cout << "MON";      break;
                case 2:     cout << "TUE";      break;
                case 3:     cout << "WED";      break;
                case 4:     cout << "THU";      break;
                case 5:     cout << "FRI";      break;
                case 6:     cout << "SAT";      break;
            }
            cout << " - " << hm_to_string(&note.start) << " " <<
            hm_to_string(&note.end);
            break;

        case Note::DAILY:
            cout << "DAILY - - " << hm_to_string(&note.start) << " " <<
                hm_to_string(&note.end);
            break;

        case Note::NOW:
            cout << "NOW - - - -";
            break;

        case Note::USE_BRIGHT:
            cout << "USE_BRIGHT - - " << hm_to_string(&note.start) << " " <<
                hm_to_string(&note.end);
            break;

        case Note::DIM_VALUE:
            cout << "DIM_VALUE - - - - " <<
                hm_to_string(&note.end);
            break;
    }

    cout << " \"" << note.text << "\"" << endl;
}

bool parse_note(string *str, Note *note)
{
    // ONE_TIME 0000-00-00 0000-00-00 00:00 00:00 text
    // DATE 00-00 00-00 00:00 00:00 text
    // DATE 00-00 00-00 - - text
    // DAY_OF_WEEK Thu - 00:00 00:00 text
    // NOW - - - - text

    istringstream iss(*str);
    bool ok = true;

    note->note_type = Note::NOW;
    note->day_of_week = 0;
    note->start = day_start;
    note->end = day_end;
    note->text = "";

    string token;
    iss >> token;

    // date type
    if (token.compare("DATE") == 0) note->note_type = Note::DATE;
    else if (token.compare("ONE_TIME") == 0) note->note_type = Note::ONE_TIME;
    else if (token.compare("DAY_OF_WEEK") == 0) note->note_type = Note::DAY_OF_WEEK;
    else if (token.compare("DAILY") == 0) note->note_type = Note::DAILY;
    else if (token.compare("NOW") == 0) note->note_type = Note::NOW;
    else if (token.compare("USE_BRIGHT") == 0) note->note_type = Note::USE_BRIGHT;
    else if (token.compare("DIM_VALUE") == 0) note->note_type = Note::DIM_VALUE;
    else ok = false;

    // from date
    if (ok) {
        iss >> token;

        switch (note->note_type) {
            case Note::DATE:
                ok = parse_md(token, &note->start);
                break;

            case Note::ONE_TIME:
                ok = parse_ymd(token, &note->start);
                break;

            case Note::DAY_OF_WEEK:
                if (token.compare("SUN") == 0) note->day_of_week = 0;
                else if (token.compare("MON") == 0) note->day_of_week = 1;
                else if (token.compare("TUE") == 0) note->day_of_week = 2;
                else if (token.compare("WED") == 0) note->day_of_week = 3;
                else if (token.compare("THU") == 0) note->day_of_week = 4;
                else if (token.compare("FRI") == 0) note->day_of_week = 5;
                else if (token.compare("SAT") == 0) note->day_of_week = 6;
                else ok = false;
                break;

            case Note::DAILY:
                break;

            case Note::NOW:
                break;

            case Note::USE_BRIGHT:
                break;

            case Note::DIM_VALUE:
                break;
        }
    }

    // to date
    if (ok) {
        iss >> token;

        if (token.compare("-") == 0) {
            note->end = note->start;

        } else {
            switch (note->note_type) {
                case Note::DATE:
                    if (token.compare("-") == 0) note->end = note->start;
                    else ok = parse_md(token, &note->end);
                    break;

                case Note::ONE_TIME:
                    if (token.compare("-") == 0) note->end = note->start;
                    else ok = parse_ymd(token, &note->end);
                    break;

                case Note::DAY_OF_WEEK:
                    break;

                case Note::DAILY:
                    break;

                case Note::NOW:
                    break;

				case Note::USE_BRIGHT:
					break;

				case Note::DIM_VALUE:
					break;
            }
        }
    }

    // from time
    if (ok) {
        iss >> token;

        if (token.compare("-") == 0) {
            copy_hms(&day_start, &note->start);

        } else {
            tm from_time;
            ok = parse_hm(token, &from_time);
            copy_hms(&from_time, &note->start);
        }
    }

    // to time
    if (ok) {
        iss >> token;

        if (token.compare("-") == 0) {
            copy_hms(&day_end, &note->end);

        } else {
            tm to_time;
            ok = parse_hm(token, &to_time);
            to_time.tm_sec = 59;
            copy_hms(&to_time, &note->end);
        }
   }

    // text
    if (ok) {
        char c;
        iss >> c;
        iss.putback(c);
        getline(iss, note->text);
    }

    return ok;
}

bool is_note_printable(Note *note)
{
	switch(note->note_type) {
        case Note::DATE:
        case Note::ONE_TIME:
        case Note::DAY_OF_WEEK:
        case Note::DAILY:
        case Note::NOW:
        	return true;
        default:
        	return false;
	}
}

bool is_note_active(Note *note, tm *at_time)
{
    bool active = true;

    switch (note->note_type) {
        case Note::DATE:
            active = true;

            if (at_time->tm_mon < note->start.tm_mon) {
                active = false;

            } else if (at_time->tm_mon > note->start.tm_mon) {
                // start OK

            } else if (at_time->tm_mday < note->start.tm_mday) {
                active = false;

            } else if (at_time->tm_mday > note->start.tm_mday) {
                // start OK

            } else if (at_time->tm_hour < note->start.tm_hour) {
                active = false;

            } else if (at_time->tm_hour > note->start.tm_hour) {
                // start OK

            } else if (at_time->tm_min < note->start.tm_min) {
                active = false;

            } else if (at_time->tm_min > note->start.tm_min) {
                // start OK

            } else if (at_time->tm_sec < note->start.tm_sec) {
                active = false;

            } else  {
                // start OK
            }

            if (at_time->tm_mon > note->end.tm_mon) {
                active = false;

            } else if (at_time->tm_mon < note->end.tm_mon) {
                // end OK

            } else if (at_time->tm_mday > note->end.tm_mday) {
                active = false;

            } else if (at_time->tm_mday < note->end.tm_mday) {
                // end OK

            } else if (at_time->tm_hour > note->end.tm_hour) {
                active = false;

            } else if (at_time->tm_hour < note->end.tm_hour) {
                // end OK

            } else if (at_time->tm_min > note->end.tm_min) {
                active = false;

            } else if (at_time->tm_min < note->end.tm_min) {
                // end OK

            } else if (at_time->tm_sec > note->end.tm_sec) {
                active = false;

            } else  {
                // end OK
            }
            break;

        case Note::ONE_TIME:
            active = true;

            if (at_time->tm_year < note->start.tm_year) {
                active = false;

            } else if (at_time->tm_year > note->start.tm_year) {
                // start OK

            } else if (at_time->tm_mon < note->start.tm_mon) {
                active = false;

            } else if (at_time->tm_mon > note->start.tm_mon) {
                // start OK

            } else if (at_time->tm_mday < note->start.tm_mday) {
                active = false;

            } else if (at_time->tm_mday > note->start.tm_mday) {
                // start OK

            } else if (at_time->tm_hour < note->start.tm_hour) {
                active = false;

            } else if (at_time->tm_hour > note->start.tm_hour) {
                // start OK

            } else if (at_time->tm_min < note->start.tm_min) {
                active = false;

            } else if (at_time->tm_min > note->start.tm_min) {
                // start OK

            } else if (at_time->tm_sec < note->start.tm_sec) {
                active = false;

            } else  {
                // start OK
            }

            if (at_time->tm_year > note->end.tm_year) {
                active = false;

            } else if (at_time->tm_year < note->end.tm_year) {
                // end OK

            } else if (at_time->tm_mon > note->end.tm_mon) {
                active = false;

            } else if (at_time->tm_mon < note->end.tm_mon) {
                // end OK

            } else if (at_time->tm_mday > note->end.tm_mday) {
                active = false;

            } else if (at_time->tm_mday < note->end.tm_mday) {
                // end OK

            } else if (at_time->tm_hour > note->end.tm_hour) {
                active = false;

            } else if (at_time->tm_hour < note->end.tm_hour) {
                // end OK

            } else if (at_time->tm_min > note->end.tm_min) {
                active = false;

            } else if (at_time->tm_min < note->end.tm_min) {
                // end OK

            } else if (at_time->tm_sec > note->end.tm_sec) {
                active = false;

            } else  {
                // end OK
            }
            break;

        case Note::DAY_OF_WEEK:
            active = at_time->tm_wday == note->day_of_week;

            if (at_time->tm_hour < note->start.tm_hour) {
                active = false;

            } else if (at_time->tm_hour > note->start.tm_hour) {
                // start OK

            } else if (at_time->tm_min < note->start.tm_min) {
                active = false;

            } else if (at_time->tm_min > note->start.tm_min) {
                // start OK

            } else if (at_time->tm_sec < note->start.tm_sec) {
                active = false;

            } else  {
                // start OK
            }

            if (at_time->tm_hour > note->end.tm_hour) {
                active = false;

            } else if (at_time->tm_hour < note->end.tm_hour) {
                // end OK

            } else if (at_time->tm_min > note->end.tm_min) {
                active = false;

            } else if (at_time->tm_min < note->end.tm_min) {
                // end OK

            } else if (at_time->tm_sec > note->end.tm_sec) {
                active = false;

            } else  {
                // end OK
            }
            break;

        case Note::DAILY:
        case Note::USE_BRIGHT:
            active = true;

            if (at_time->tm_hour < note->start.tm_hour) {
                active = false;

            } else if (at_time->tm_hour > note->start.tm_hour) {
                // start OK

            } else if (at_time->tm_min < note->start.tm_min) {
                active = false;

            } else if (at_time->tm_min > note->start.tm_min) {
                // start OK

            } else if (at_time->tm_sec < note->start.tm_sec) {
                active = false;

            } else  {
                // start OK
            }

            if (at_time->tm_hour > note->end.tm_hour) {
                active = false;

            } else if (at_time->tm_hour < note->end.tm_hour) {
                // end OK

            } else if (at_time->tm_min > note->end.tm_min) {
                active = false;

            } else if (at_time->tm_min < note->end.tm_min) {
                // end OK

            } else if (at_time->tm_sec > note->end.tm_sec) {
                active = false;

            } else  {
                // end OK
            }
            break;

        case Note::NOW:
        case Note::DIM_VALUE:
            active = true;
            break;
    }

    return active;
}

#if 0
int main(int argc, const char * argv[]) {
    notes.clear();

    ifstream ifs("foo.txt");

    if (ifs.is_open()) {
        cout << "open" << endl;
        string str;
        bool ok = true;
        do {
            Note note;
            getline(ifs, str);
            ok = parse_note(&str, &note);
            if (ok) notes.push_back(note);

        } while (!ifs.eof() && ok);

        ifs.close();
    }

    time_t now;
    tm *local_tm;
    now = time(0);
    local_tm = localtime(&now);

    for (int k = 0; k < notes.size(); k++) {
        Note note = notes.at(k);
        cout << (is_note_active(&note, local_tm) ? "Yes " : "No  ");
        print_note(note);
    }

    return 0;
}
#endif