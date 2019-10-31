#ifndef note_hpp
#define note_hpp

#include <string>
#include <ctime>

struct Note {
    enum {
        DATE,
        ONE_TIME,
        DAY_OF_WEEK,
        DAILY,
        NOW,
        USE_BRIGHT,
        DIM_VALUE
    } note_type;

    int day_of_week;

    tm start;
    tm end;

    std::string text;
};
typedef struct Note Note;

bool is_digit(char c);
bool parse_ymd(const std::string str, tm *ymd);
bool parse_md(const std::string str, tm *ymd);
bool parse_hm(const std::string str, tm *ymd);
void copy_hms(const tm *from, tm *to);
std::string hm_to_string(tm *the_tm);
std::string md_to_string(tm *the_tm);
std::string ymd_to_string(tm *the_tm);
void print_note(Note note);
bool parse_note(std::string *str, Note *note);
bool is_note_printable(Note *note);
bool is_note_active(Note *note, tm *at_time);

#endif /* sound_io_hpp */
