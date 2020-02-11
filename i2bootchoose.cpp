#include <stdlib.h>
#include <getopt.h>
#include <linux/types.h>

#include<iostream>
#include<fstream>
#include<string>
#include<list>
#include<vector>


using namespace std;

int set_index = 0;
bool set_flag = false;
bool list_flag = false;

std::string& trim(std::string &s)
{
	if (s.empty())
	{
		return s;
	}
	s.erase(0, s.find_first_not_of(" \t"));
	s.erase(s.find_last_not_of(" \t") + 1);
	return s;
}
bool startswith(const std::string& str, const std::string& start)
{
	int srclen = str.size();
	int startlen = start.size();
	if(srclen >= startlen)
	{
		string temp = str.substr(0, startlen);
		if(temp == start)
			return true;
	}

	return false;
}

vector<string> split(string str, string pattern)
{
	string::size_type pos;
	vector<string> result;

	str += pattern;
	int size = str.size();

	for (int i = 0; i<size; i++) {
		pos = str.find(pattern, i);
		if (pos<size) {
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

class config_label
{

public:
	config_label() {}
	string text_label;
	string text_kernel;
	string text_fdt;
	string text_append;
};

class config_data
{
public:
	bool is_enable;
	config_data(const string &file_name);
	string name;
	string text_head;
	string text_default_label;
	vector<config_label *> list_label;
};

config_data::config_data(const string &file_name)
{
	bool read_label_flag = false;
	config_label *new_label;
	fstream f;
	string line;
	f.open(file_name.c_str());
	while(getline(f,line))
	{
		line = trim(line);
		if (startswith(line, "LABEL")) {
			read_label_flag = true;
		}

		if (startswith(line, "LABEL") || read_label_flag) {

			if (startswith(line, "LABEL")) {
				new_label = new config_label();
				new_label->text_label = line;
			} else if (startswith(line, "KERNEL")) {
				if (new_label)
					new_label->text_kernel = line;
			} else if (startswith(line, "FDT")) {
				if (new_label)
					new_label->text_fdt = line;
			} else if (startswith(line, "APPEND")) {
				if (new_label)
					new_label->text_append = line;
				list_label.push_back(new_label);
				new_label = NULL;
			}
		} else if (startswith(line, "DEFAULT") || read_label_flag) {
			text_default_label = line;
		} else {
			text_head += line + "\n";
		}
	}
	f.close();

	is_enable = true;
	if (text_head.empty() || text_default_label.empty() || (list_label.size() == 0))
		is_enable = false;
}


void config_label_show(const string &file_name)
{
	config_data *config = new config_data(file_name);
	if (!config->is_enable) {
		cout << file_name << " formats are not supported\n";
		return;
	}

	string default_label = split(config->text_default_label, " ").at(1);
	for (int i = 0; i != config->list_label.size(); ++i )
	{
		config_label *label_new = config->list_label.at(i);

		if (default_label == split(label_new->text_label, " ").at(1))
			printf("* %d.%s\n", i + 1, split(label_new->text_label, " ").at(1).c_str());
		else
			printf("  %d.%s\n", i + 1, split(label_new->text_label, " ").at(1).c_str());
	}
}

void config_label_set(const string &file_name, unsigned int index)
{
	config_data *config = new config_data(file_name);
	string new_default_label;

	if (!config->is_enable) {
		cout << file_name << " formats are not supported\n";
		return;
	}

	if (index > 0 && index <= (unsigned int)config->list_label.size()) {
		index -= 1;
		config_label *label_new = config->list_label.at(index);
		new_default_label = split(label_new->text_label, " ").at(1);
	} else {
		printf("index[%d] is invaild\n", index);
		return;
	}


	ofstream destFile(file_name.c_str(), ios::out);
	if(!destFile) {
		cout << "error opening " << file_name << endl;
		return;
	}
	destFile << config->text_head;
	if (new_default_label.empty())
		destFile << config->text_default_label << endl;
	else
		destFile << "DEFAULT " << new_default_label << endl;

	for (int i=0; i!=config->list_label.size(); ++i )
	{
		config_label *label_new = config->list_label.at(i);
		destFile << label_new->text_label << endl;
		destFile << "\t" << label_new->text_kernel << endl;
		destFile << "\t" << label_new->text_fdt << endl;
		destFile << "\t" << label_new->text_append << endl;
	}
	destFile.close();
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-sl]\n", prog);
	puts("  -s --set  set boot item index for current system(e.g. \"-s 1\"))\n"
	     "  -l --list list boot item on current system\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "set",  1, 0, 's' },
			{ "list",   0, 0, 'l' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "s:l", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 's':
			set_index = atoi(optarg);
            set_flag = true;
			break;
		case 'l':
			list_flag = true;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
    if (argc == 1)
        print_usage(argv[0]);
}

int main(int argc, char *argv[])
{
	parse_opts(argc, argv);


	string file_name = "./extlinux.conf";
	fstream f;
	string line;
	string::size_type idx;

	f.open("/proc/cmdline");
	if (!f) {
		cout << "/proc/cmdline open failed\n";
		return 0;
	}
	getline(f,line);
	line = trim(line);

	idx = line.find("mmcblk0");
	if(idx == string::npos )
		file_name = "/boot/mmc1_panguboard_extlinux/extlinux.conf";
	else
		file_name = "/boot/mmc0_panguboard_extlinux/extlinux.conf";
	f.close();
	cout << "file_name: " << file_name << "\n";

	if (list_flag) {
		config_label_show(file_name);
		return 0;
	}
	if (set_flag) {
		config_label_set(file_name, set_index);
		config_label_show(file_name);
	}
}
