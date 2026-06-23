#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <climits>
#include <cctype>
#include <fstream>
#include <sstream>

using namespace std;

// default table size
const unsigned int DEFAULT_SIZE = 179;

// A structure to hold course information
struct Course {
	string course_number;
	string course_title;
	vector<string> prerequisites;
};

class HashTable {
private:
	// Define structures to hold courses
	struct Node {
		Course course;
		Node* next;
		unsigned long long key;

		// default constructor
		Node() {
			course = { "", "", {} };
			next = nullptr;
			key = UINT_MAX; // Use UINT_MAX to indicate an empty slot
		}

		// intialize with a course
		Node(Course aCourse) : Node() {
			course = aCourse;
		}

		// initialize with a course and a key
		Node(Course aCourse, unsigned long long aKey) : Node(aCourse) {
			key = aKey;
		}
	};

	vector<Node> nodes;	// chaining for collisions

	unsigned int tableSize = DEFAULT_SIZE;

	unsigned int hash(unsigned long long key);

public:
	HashTable();
	HashTable(unsigned int size);
	void LoadFile(string csvPath);
	Course Search(string course_number);
	void PrintAll();
	size_t size = 0;

};


	// Default constructor
	HashTable::HashTable() {
		nodes.resize(tableSize);
	}

	// constructor to initialize the hash table with a specific size
	HashTable::HashTable(unsigned int size) {
		this->tableSize = size;

		// resize nodes size
		nodes.resize(tableSize);
	}

	// Hash function to compute the hash value for a given key
	unsigned int HashTable::hash(unsigned long long key) {
		return key % tableSize;
	}

	void HashTable::LoadFile(string csvPath) {
		ifstream file(csvPath);
		string line;


		if (!file.is_open()) {
			cerr << "Error opening file: " << csvPath << endl;
			cout << endl;
			return;
		}
		else {
			cout << "file opened successfully" << endl;
			cout << endl;
		}


		// count the lines in the file to determine the size of the hash table
		int line_count = 0;
		while (getline(file, line)) {
			line_count++;
			tableSize = line_count;	// set the table size to the number of lines in the file
		}

		// resize the hash table to the number of lines in the file
		nodes.resize(tableSize);


		file.clear();	// reset file to the beginning after counting lines
		file.seekg(0);


		// 1. read the first element on the line (course_number) and hash it to determine the index of the course in the hash table
		while (getline(file, line)) {
			stringstream ss(line);
			string course_number;
			
			if (getline(ss, course_number, ',')) { // read the first element (course_number)

				// normalize course number to uppercase for consistent storage/comparison
				for (char &c : course_number) c = toupper(static_cast<unsigned char>(c));
			}


			unsigned long long key = 0;							


			for (char c : course_number) {						// loop over character and add their values to create a unique key for the course number
				if (isalpha(c)) {
					key += static_cast<unsigned long long>(c); // convert letters
				}
				else if (isdigit(c)) {
					key += c - '0';							// subract 0 to get actual numerical value of a letter character and not its ASCII value
				}
			}

			key = key * 31;									// multiply by a prime number to further reduce the chances of collisions
			unsigned int index = this->hash(key);					
			
			Course init_course;
			init_course.course_number = course_number;

			if (nodes.at(index).key == UINT_MAX) {
				nodes.at(index).course = init_course;			// if the slot is empty, insert the course
				nodes.at(index).key = key;
				nodes.at(index).next = nullptr;
			}
			else {	// collision occurred, create a new node and insert it at the beginning of the linked list at that index

				Node* new_node = new Node(init_course, key);

				new_node->next = nodes.at(index).next;
				nodes.at(index).next = new_node;
			}
		}

		file.clear();
		file.seekg(0);


		/* 2. read the first element on the line(course_number) and find its matching index in the hash table
		      read the second element on the line (course_title) and store it in the course_title field of the matching course struct at that index in the hash table
		      if there are more than two elements on the line, (prerequisites exist) then,
		      match the prerequisite course number against the course numbers in the hash table to verify that they exist, and if validated, store them in the prereqs vector at that index in the hash table
		*/
		while (getline(file, line)) {
			stringstream ss(line);
			string course_number;

			if (!getline(ss, course_number, ',')) {
				continue;							// if the line is empty, skip to the next line
			}

			unsigned long long key = 0;

			for (char c : course_number) {						
				if (isalpha(c)) {
					key += static_cast<unsigned long long>(c); 
				}
				else if (isdigit(c)) {
					key += c - '0';							
				}
			}

			key = key * 31;									
			unsigned int index = this->hash(key);
			
			Node* current = &nodes.at(index);

			// if current bucket at index is occupied, traverse to end of list or until course_number is found
			while (current->key != UINT_MAX && current->course.course_number != course_number) {

				if (current->next == nullptr) {
					current->next = new Node();	// allocate new node at end of list
					current = current->next;
					current->key = UINT_MAX;
					break;
				}
				current = current->next;

			}

			current->key = key;	// set the key for the current node
			current->course.course_number = course_number;

			string course_title;
			if (getline(ss, course_title, ',')) {
				current->course.course_title = course_title;	// store the course title in the course struct at the index
			}

			// Second pass to read the prerequisites and validate that they started a line in the file
			// by matching the course number against those collected from each line on the first pass and stored as an index in the hash table

			string prereq;
			while (getline(ss, prereq, ',')) {
				if (prereq.empty()) continue;	// skip trailing commas

				unsigned long long prereq_key = 0;

				for (char c : prereq) {	// iterate over each character in the course number, convert letters to their ASCII values and numbers to their actual values
					if (isalpha(c)) {
						prereq_key += static_cast<unsigned long long>(c);
					}
					else if (isdigit(c)) {
						prereq_key += c - '0';
					}
				}

				prereq_key = prereq_key * 31;						// multiply by prime number to facilite unique key generation
				unsigned int prereq_index = this->hash(prereq_key);

				bool prereq_exists = false;

				Node* search_node = &nodes.at(prereq_index);

				if (current != nullptr) {
					while (search_node != nullptr && search_node->key != UINT_MAX) {
						if (search_node->course.course_number == prereq) {
							prereq_exists = true;
							break;
						}
						search_node = search_node->next;
					}

					if (prereq_exists) {
							
						// if the prerequisite course exists, add it to the prereqs vector in the course struct at the index
							current->course.prerequisites.push_back(prereq);
					} else {
						cout << "Prerequisite course " << prereq << " does not start a line in the file " << course_number << endl;	
					}
				}
			}

	

	}
		file.close();		
		
	}
	

	// Print all courses in alphanumeric order
	void HashTable::PrintAll() {

		// 1. Gather all valid courses into a vector
		vector<Course> allCourses;

		for (unsigned int i = 0; i < tableSize; i++) {
			Node* current = &nodes.at(i);

			while (current != nullptr) {

				// Check if the current node contains a valid course
				if (current->key != UINT_MAX && !current->course.course_number.empty()) {
					allCourses.push_back(current->course);
				}
				current = current->next;
			}
		}

		// 2. Sort the vector alphanumerically by course_number
		sort(allCourses.begin(), allCourses.end(), [](const Course& a, const Course& b) {
			return a.course_number < b.course_number;
			});

		// 3. Print the sorted courses
		if (allCourses.empty()) {
			cout << "No courses found in the system." << endl;
			return;
		}

		for (const auto& course : allCourses) {
			cout << "--------------------------------------------" << endl;
			cout << "Course Number: " << course.course_number << endl;
			cout << "Course Title: " << course.course_title << endl;
			cout << "Prerequisites: ";

			if (course.prerequisites.empty()) {
				cout << "None";
			}
			else {
				for (size_t j = 0; j < course.prerequisites.size(); j++) {
					cout << course.prerequisites[j];
					if (j < course.prerequisites.size() - 1) {
						cout << ", ";
					}
				}
			}
			cout << endl << "--------------------------------------------" << endl;
		}
	}


	// function to search for a course by course_number
	Course HashTable::Search(string course_number) {
		
			// normalize input to uppercase to match stored values
			for (char &c : course_number) c = toupper(static_cast<unsigned char>(c));

			unsigned long long key = 0;

			for (char c : course_number) {	// build key
				if (isalpha(c)) key += static_cast<unsigned long long>(c);
				else if (isdigit(c)) key += c - '0';
			}

			key = key * 31;
			unsigned int index = hash(key);

			// retrieve node using index
			Node* current = &nodes.at(index);

			while (current != nullptr) {
				// skip empty bucket entries
				if (current->key == UINT_MAX) {
					current = current->next;
					continue;
				}

				// check for matching course in the chain at index
				if (current->key == key && current->course.course_number == course_number) {
					cout << "--------------------------------------------------" << endl;
					cout << "Course Number: " << current->course.course_number << endl;
					cout << "Course Title: " << current->course.course_title << endl;
					cout << "Prerequisites: ";

					if (current->course.prerequisites.empty()) {
						cout << "none";
					}

					for (size_t j = 0; j < current->course.prerequisites.size(); ++j) {
						cout << current->course.prerequisites[j];
						if (j + 1 < current->course.prerequisites.size()) cout << ", ";
					}

					cout << endl;
					cout << "--------------------------------------------------" << endl;
					cout << endl;
					return current->course;
				}

				// if not a match then move to the next node
				current = current->next;
			}

			// not found: prompt user for a valid course number and loop
			cout << "Course does not exist." << endl << "Enter a valid course number : " << endl;
			cin >> course_number;
			cout << endl;

			return Course();


	}
	

	int main() {
		string user_input;
		int selection = 0;

		// Define a hash table to hold all the courses
		HashTable* courseTable;

		Course course;
		courseTable = new HashTable();

		while (selection != 9) {
			// main menu user interface
			cout << "----------------------" << endl;							// formatting for better readability of the menu
			cout << "----------------------" << endl;
			cout << "         MENU" << endl;
			cout << "   Select an Option" << endl;
			cout << endl;
			cout << "1.     Load File" << endl;
			cout << "2.   Search Course" << endl;
			cout << "3. Print All Courses" << endl;
			cout << "9.       Exit" << endl;
			cout << "----------------------" << endl;							// formatting for better readability of the menu
			cout << "----------------------" << endl;

			cin >> selection;

			if (selection != 1 && selection != 2 && selection != 3 && selection != 9) { // selection validation
				cout << "Enter a valid selection" << endl;
				cin >> selection;
			}

			switch (selection) {		
			case 1:												// case 1: load a file
				cout << "Enter file path to load file" << endl;
				cin >> user_input; 

				cout << "loading file..." << endl;
				cout << endl;

				courseTable->LoadFile(user_input);
				break;

			case 2:
				cout << "Enter course number to search" << endl;// case 2: search for a course with a course number
				cout << endl;
				cin >> user_input;

				for (char& c : user_input) {
					c = toupper(static_cast<unsigned char>(c));	// user_text to uppercase letters
				}
				
				course = courseTable->Search(user_input);
				break;

			case 3:
				cout << "   Printing all courses..." << endl;// case 3: print all courses and their prerequisite information
				cout << endl;
				courseTable->PrintAll();
				cout << endl;
				break;

			case 9:
				cout << "Exiting the program....Goodbye." << endl;			// case 9: exit the program
				return 0;
			}

		}
	}

