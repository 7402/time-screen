/*
Install library
sudo apt-get install libcairo2-dev

Release build: [change myserver.xyz to server domain]
# g++ -DSERVER=myserver.xyz -Wall -g time-screen.cpp note.cpp -I/usr/include/cairo -L/usr/lib/arm-linux-gnueabihf/ -lcairo -o time-screen

Development build: [change myserver.xyz to server domain]
# g++ -DDEV -DSERVER=myserver.xyz -Wall -g time-screen.cpp note.cpp -I/usr/include/cairo -L/usr/lib/arm-linux-gnueabihf/ -lcairo -o time-screend

# turn off blinking cursor
sudo su - -c "echo 0 > /sys/class/graphics/fbcon/cursor_blink"

./time-screen
or
./time-screend 20
./time-screend

# turn on blinking cursor
sudo su - -c "echo 1 > /sys/class/graphics/fbcon/cursor_blink"
*/

#include <cairo.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>

// #include <libssh/sftp.h>

#include "note.hpp"

#define STRING2(x) #x
#define STRING(x) STRING2(x)

using namespace std;

vector<Note> notes;

void show_centered_text(cairo_t *cr, const char *text, double x, double *y, int font_size, double line);
void show_centered_text(cairo_t *cr, const char *text, double x, double *y, int font_size, double line)
{
	cairo_set_font_size(cr, font_size);

	cairo_text_extents_t extents;
	
	cairo_text_extents (cr, text, &extents);		    
	cairo_move_to(cr, x - extents.width / 2, *y);
	cairo_show_text(cr, text);
	
	*y += line;
}

void show_centered_text2(cairo_t *cr, const char *text1, const char *text2, double x, double *y,
	int font_size1, int font_size2, double line);
void show_centered_text2(cairo_t *cr, const char *text1, const char *text2, double x, double *y,
	int font_size1, int font_size2, double line)
{
	cairo_set_font_size(cr, font_size2);
	cairo_text_extents_t extents2;
	cairo_text_extents (cr, text2, &extents2);
			    
	cairo_set_font_size(cr, font_size1);
	cairo_text_extents_t extents1;
	cairo_text_extents (cr, text1, &extents1);
			    
	cairo_move_to(cr, x - (extents1.width + extents2.width) / 2, *y);
	cairo_show_text(cr, text1);

	cairo_move_to(cr, x - (extents1.width + extents2.width) / 2 + extents1.width, *y);
	cairo_set_font_size(cr, font_size2);
	cairo_show_text(cr, text2);
	
	*y += line;
}

bool draw_info(cairo_t *cr);
bool draw_info(cairo_t *cr)
{
	bool showing_ip = false;

	cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
	cairo_set_font_size(cr, 20);

	time_t now;
    tm *local_tm;
    now = time(0);
    local_tm = localtime(&now);
	
	string text = local_tm->tm_zone;
	
	struct ifaddrs *ifap = nullptr;
    if (0 == getifaddrs(&ifap)) {
		struct ifaddrs *next = ifap;
		while (next != nullptr) {
			if (next->ifa_addr != nullptr && next->ifa_addr->sa_family == AF_INET) {
           		char host[NI_MAXHOST];
                if (0 == getnameinfo(next->ifa_addr, sizeof(struct sockaddr_in),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST)) {
                    if (strcmp(host, "127.0.0.1") != 0) {
						text += " ";
                    	text += host;
                    	showing_ip = true;
                    }
                }
			}
			
			next = next->ifa_next;
		}
    
    	freeifaddrs(ifap);
    }
	
	cairo_move_to(cr, 20, 310);
	cairo_show_text(cr, text.c_str());
	
	return showing_ip;
}

void draw_notes(cairo_t *cr, double dim);
void draw_notes(cairo_t *cr, double dim)
{
	int note_count = 0;
	string active_notes[3];

	time_t now;
    tm *local_tm;
    now = time(0);
    local_tm = localtime(&now);
	
	for (size_t k = 0; k < notes.size() && note_count < 3; k++) {
		Note note = notes.at(k);
        if (is_note_active(&note, local_tm) && is_note_printable(&note)) {
        	active_notes[note_count] = "• ";
        	active_notes[note_count] += note.text;
        	note_count++;
        }
        
        //print_note(note);
    }

	if (note_count == 0) {
		cairo_set_source_rgb(cr, dim * 0.1, dim * 0.1, dim * 0.2);

	} else {
		cairo_set_source_rgb(cr, dim * 0.55, dim * 0.55, dim * 1.0);
	}	
	
	const int MARGIN = 20;
	const int X = 50;
	const int Y = 320;

	cairo_set_line_width(cr, 2.0);
	cairo_rectangle(cr, MARGIN, Y, 1024 - 2 * MARGIN, 600 - Y - MARGIN);
	
	cairo_set_font_size(cr, 48);

	double y = Y + 1.5 * 48;

	if (note_count >= 1) {
		cairo_move_to(cr, X, y);
		cairo_show_text(cr, active_notes[0].c_str());
	}
	
	if (note_count >= 2) {
		y += 1.5 * 48;
		cairo_move_to(cr, X, y);
		cairo_show_text(cr, active_notes[1].c_str());
	}
	
	if (note_count >= 3) {
		y += 1.5 * 48;
		cairo_move_to(cr, X, y);
		cairo_show_text(cr, active_notes[2].c_str());
	}
	
	cairo_stroke(cr);
}

void draw_time(cairo_t *cr, double dim);
void draw_time(cairo_t *cr, double dim)
{
	time_t now;
	struct tm *local_tm;
	const size_t BUF_LEN = 128;
	char time_str[BUF_LEN];
	ostringstream oss;
	ostringstream oss2;

	now = time(0);
	local_tm = localtime(&now);

	if (0 != strftime(time_str, BUF_LEN, "%A", local_tm)) {
		oss << time_str << " ";
	}
	
	//printf("local_tm->tm_hour = %d\n", local_tm->tm_hour);

	if (local_tm->tm_hour < 4) {
		oss << "before sunrise";

	} else if (local_tm->tm_hour < 6) {
		oss << "early Morning";

	} else if (local_tm->tm_hour < 12) {
		oss << "Morning";

	} else if (local_tm->tm_hour < 17) {
		oss << "Afternoon";

	} else if (local_tm->tm_hour < 21) {
		oss << "Evening";

	} else {
		oss << "Night";
	}

	cairo_select_font_face(cr, "DejaVuSans",
							CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	double x = 512;
	double y = 80;

	cairo_set_source_rgb(cr, dim * 1.0, dim * 1.0, dim * 1.0);

	show_centered_text(cr, oss.str().c_str(), x, &y, 60, 80);
	oss.str("");
				

	if (0 != strftime(time_str, BUF_LEN,
			local_tm->tm_mday < 10 ? "%B%e, %Y" : "%B %e, %Y", local_tm)) {
		oss << time_str;
	}

		cairo_set_source_rgb(cr, dim * 1.0, dim * 1.0, dim * 0.0);

		show_centered_text(cr, oss.str().c_str(), x, &y, 48, 96);
		oss.str("");
		oss2.str("");

	if (0 != strftime(time_str, BUF_LEN,"%I:%M", local_tm)) {
		if ((local_tm->tm_hour > 0 && local_tm->tm_hour <= 9) ||
			(local_tm->tm_hour > 12 && local_tm->tm_hour <= 21)) {

			oss << (time_str + 1);

		} else {
			oss << time_str;
		}
	}

	if (0 != strftime(time_str, BUF_LEN,"  %p", local_tm)) {
		oss2 << time_str;
	}

	cairo_set_source_rgb(cr, dim * 1.0, dim * 1.0, dim * 1.0);

	show_centered_text2(cr, oss.str().c_str(), oss2.str().c_str(), x, &y, 80, 40, 96);
	oss.str("");

	cairo_stroke(cr);
}

void read_local_notes(string path);
void read_local_notes(string path)
{
    notes.clear();

    ifstream ifs(path);

    if (ifs.is_open()) {
        //cout << "open" << endl;
        string str;
        bool ok = true;
        do {
            Note note;
            getline(ifs, str);
            if (str.length() > 0 && str.at(0) != '#') {
				ok = parse_note(&str, &note);
				if (ok) notes.push_back(note);
            }

        } while (!ifs.eof() && ok);

        ifs.close();
    }
}

int main(int argc, char **argv) {
	int run_for_secs = 0;

	if (argc > 1) {
		run_for_secs = atoi(argv[1]);
		printf("run_for_secs = %d\n", run_for_secs);
	}
	
    cairo_t *cr = NULL;
    cairo_surface_t *surface = NULL;
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    unsigned char *fbp = 0;

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    //printf("The framebuffer device was opened successfully.\n");
    
    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }
    
    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }
    
    /*
    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    
    printf("red offset=%d length=%d msb_right=%d\n", vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right);
    printf("green offset=%d length=%d msb_right=%d\n", vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right);
    printf("blue offset=%d length=%d msb_right=%d\n", vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right);
    printf("transp offset=%d length=%d msb_right=%d\n", vinfo.transp.offset, vinfo.transp.length, vinfo.transp.msb_right);

    printf("line_length = %d bytes\n", finfo.line_length);
    */
    
    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                                fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    //printf("The framebuffer device was mapped to %ld bytes of memory successfully.\n", screensize);

	size_t buf_len = vinfo.yres * finfo.line_length;
	void *saved = malloc(buf_len);
	memcpy(saved, fbp, buf_len);

    surface = cairo_image_surface_create_for_data(fbp, CAIRO_FORMAT_ARGB32, vinfo.xres, vinfo.yres, finfo.line_length);    
    cr = cairo_create(surface);
    
	time_t now = time(0);
    time_t start_time = now;
    
    string old_text = "";
    
    bool sent_initial_screen = false;

	for (int k = 0; now < start_time + run_for_secs || run_for_secs == 0; k++) {
		read_local_notes("/home/pi/Projects/notes.txt");

		now = time(0);
		struct tm *local_tm;
		local_tm = localtime(&now);
		bool first_2_min = now - start_time < 120;
		
		bool use_dim = local_tm->tm_hour < 7 || local_tm->tm_hour >= 20;
		double dim_value = 0.10;
		
		string new_text = "";
		
		for (size_t k = 0; k < notes.size(); k++) {
			Note note = notes.at(k);
			if (note.note_type == Note::USE_BRIGHT) {
				use_dim = !is_note_active(&note, local_tm);
			}
			
			if (note.note_type == Note::DIM_VALUE) {
				double value = atof(note.text.c_str());
				if (value >= 0.0 && dim_value <= 1.0) dim_value = value;
			}
			
			if (is_note_active(&note, local_tm) && is_note_printable(&note)) {
				new_text += "• ";
				new_text += note.text;
			}
		}
		
		bool text_changed = old_text.compare(new_text) != 0;

		double dim = use_dim ? dim_value : 1.0;
	
		cairo_push_group(cr);

		cairo_rectangle(cr, 0, 0, vinfo.xres, vinfo.yres);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_fill(cr);
		cairo_surface_flush(surface);
		
		draw_time(cr, dim);
		cairo_surface_flush(surface);

		draw_notes(cr, dim);
		cairo_surface_flush(surface);

		bool showing_ip = false;
		if (first_2_min) showing_ip = draw_info(cr);

		cairo_pop_group_to_source(cr);
		cairo_paint_with_alpha(cr, 1.0);
		
		if (text_changed || (!sent_initial_screen && showing_ip)) {
		#ifdef DEV
			cairo_surface_write_to_png(surface, "/home/pi/Projects/screend.png");
			system("scp -q /home/pi/Projects/screend.png relay@" STRING(SERVER) ":/var/www/html/relay/ &");
		#else
			cairo_surface_write_to_png(surface, "/home/pi/Projects/screen.png");
			system("scp -q /home/pi/Projects/screen.png relay@" STRING(SERVER) ":/var/www/html/relay/ &");
		#endif
			old_text = new_text;
			sent_initial_screen = true;
		}
		
		if (first_2_min) {
			sleep(2);
			
		} else {
			int until_next_minute = 60 - local_tm->tm_sec;
			sleep(until_next_minute);
		}
	}

	cairo_rectangle(cr, 0, 0, vinfo.xres, vinfo.yres);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_fill(cr);
	cairo_surface_flush(surface);
	sleep(1);
	
    if (cr != NULL) cairo_destroy(cr);
    if (surface != NULL) cairo_surface_destroy(surface);
    
    // memcpy(fbp, saved, buf_len);


    munmap(fbp, screensize);
    close(fbfd);

    return 0;
}


