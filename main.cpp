#include <bits/stdc++.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

const int k = 2;

struct Data { //Structure
    string city;
    double lat, lng;
    string country;
    int pop;
};

struct Node {
    Data point;
    Node* left, * right;
};

Node* newNode(Data a) {
    Node* newNode = new Node;
    newNode->point = a;
    newNode->left = newNode->right = NULL;
    return newNode;
}

void getInfo(Data& tp, string temp) { //Get information from a line of a file with stringstream
    int pos;
    while ((pos = temp.find(',')) != string::npos)  temp.replace(pos, 1, " ");
    char c;
    stringstream ss(temp);
    getline(ss, tp.city, ' ');
    ss >> tp.lat >> tp.lng;
    ss.ignore();
    getline(ss, tp.country, ' ');
    getline(ss, temp, ' ');
    if (temp[0] >= '0' && temp[0] <= '9') tp.pop = stoi(temp);
    else
        tp.country += " " + temp;
    getline(ss, temp, ' ');
    if (temp[0] >= '0' && temp[0] <= '9') tp.pop = stoi(temp);
    else{
        tp.country += " " + temp;
        ss >> tp.pop;
    }
}

vector<Data> readFile(string nameFile) { //Read each line of file
    ifstream ifs(nameFile);
    string temp;
    vector<Data> ans;
    getline(ifs, temp);
    while (getline(ifs, temp)) {
        Data tp;
        getInfo(tp, temp);
        ans.push_back(tp);
    }
    return ans;
}

Node* insertRec(Node* root, Node* data, unsigned depth) { //Insert a node to KD-Tree
    if (root == NULL) {
        root = data;
        return root;
    }

    unsigned cd = depth % k; // k is the number of dimensions
    double point1, point2;

    if (cd % k == 0) { //0 for latitude
        point1 = root->point.lat;
        point2 = data->point.lat;

    }
    else { // 1 for longitude
        point1 = root->point.lng;
        point2 = data->point.lng;
    }

    if (point2 < point1)
        root->left = insertRec(root->left, data, depth + 1);
    else
        if (point2 > point1)
            root->right = insertRec(root->right, data, depth + 1);

    return root;
}

Node* KDTree(string nameFile) { //Building a tree by inserting each data was read
    vector<Data> temp = readFile(nameFile);
    Node* root = NULL;
    for (const auto& p : temp) {
        Node* tmp = newNode(p);
        root = insertRec(root, tmp, 0);
    }
    return root;
}

double haversine(double lat1, double lng1, double lat2, double lng2) { //haversine function
    const double R = 6371.0; 
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLng = (lng2 - lng1) * M_PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * sin(dLng / 2) * sin(dLng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

bool isInRangeRec(Data point, double minLat, double minLng, double maxLat, double maxLng) { //Check if a city is inside of a rectangle
    return (point.lat >= minLat && point.lat <= maxLat &&
        point.lng >= minLng && point.lng <= maxLng);
}

void rangeSearchRec(Node* root, double BottomLat, double BottomLng, double TopLat, double TopLng, unsigned depth, vector<Data>& result) {
    if (root == NULL)
        return;
    //check if a city is in rectangle
    if (isInRangeRec(root->point, BottomLat, BottomLng, TopLat, TopLng)) {
        result.push_back(root->point);
    }

    unsigned cd = depth % k;

    if ((cd == 0 && root->point.lat >= BottomLat) || (cd == 1 && root->point.lng >= BottomLng)) {
        rangeSearchRec(root->left, BottomLat, BottomLng, TopLat, TopLng, depth + 1, result);
    }

    if ((cd == 0 && root->point.lat <= TopLat) || (cd == 1 && root->point.lng <= TopLng)) {
        rangeSearchRec(root->right, BottomLat, BottomLng, TopLat, TopLng, depth + 1, result);
    }
}

//result of a range search
vector<Data> rangeSearch(Node* root, double BottomLat, double BottomLng, double TopLat, double TopLng)
{
    vector<Data> result;
    rangeSearchRec(root, BottomLat, BottomLng, TopLat, TopLng, 0, result);
    return result;
}
//find which is nearer of a given coordinate
Node* closest(Node* a, Node* b, Node* target) {
    if (a == NULL) return b;
    if (b == NULL) return a;
    double d1 = haversine(a->point.lat, a->point.lng, target->point.lat, target->point.lng);
    double d2 = haversine(b->point.lat, b->point.lng, target->point.lat, target->point.lng);
    if (d1 < d2) return a;
    else return b;
}
//find the nearest neighbor ò a give coordinate
Node* nearestNeighbor(Node* root, Node* target, int depth) {
    if (root == NULL) {
        return NULL;
    }

    Node* nextBranch = NULL;
    Node* otherBranch = NULL;
    double coordinate[2];
    coordinate[0] = target->point.lat;
    coordinate[1] = target->point.lng;
    double original[2];
    original[0] = root->point.lat;
    original[1] = root->point.lng;
    unsigned cd = depth % k;

    if (coordinate[cd] < original[cd]) {
        nextBranch = root->left;
        otherBranch = root->right;
    }
    else {
        nextBranch = root->right;
        otherBranch = root->left;
    }

    Node* tmp = nearestNeighbor(nextBranch, target, depth + 1);
    Node* best = closest(tmp, root, target);

    double distance = haversine(target->point.lat, target->point.lng, best->point.lat, best->point.lng);
    double distanceToDividedLine = coordinate[cd] - original[cd];

    if (distance >= distanceToDividedLine * distanceToDividedLine) {
        tmp = nearestNeighbor(otherBranch, target, depth + 1);
        best = closest(tmp, best, target);
    }
    return best;
}
//save the result of range search to csv
void saveToCSV(const vector<Data>& results, const string& filename) {
    ofstream ofs(filename);
    ofs << "City,Latitude,Longitude,Country,Population\n";
    for (const auto& city : results) {
        ofs << city.city << "," << city.lat << "," << city.lng << "," << city.country << "," << city.pop << "\n";
    }
    ofs.close();
}
//print the result of range search
void printResults(const vector<Data>& results) {
    for (const auto& city : results) {
        cout << city.city << " (" << city.lat << ", " << city.lng << "), " << city.country << ", Population: " << city.pop << "\n";
    }
}

json serializeNode(Node* node) {
    if (node == NULL) return nullptr;

    json j;
    j["city"] = node->point.city;
    j["lat"] = node->point.lat;
    j["lng"] = node->point.lng;
    j["country"] = node->point.country;
    j["pop"] = node->point.pop;
    j["left"] = serializeNode(node->left);
    j["right"] = serializeNode(node->right);

    return j;
}

void serializeKDTree(Node* root, const string& filename) {
    json j = serializeNode(root);
    ofstream ofs(filename);
    ofs << j.dump(4); 
    ofs.close();
    cout << "KD-Tree has been serialized to " << filename << "\n";
}

Node* deserializeNode(const json& j) {
    if (j.is_null()) return NULL;

    Data point;
    point.city = j["city"];
    point.lat = j["lat"];
    point.lng = j["lng"];
    point.country = j["country"];
    point.pop = j["pop"];

    Node* node = newNode(point);
    node->left = deserializeNode(j["left"]);
    node->right = deserializeNode(j["right"]);

    return node;
}

Node* deserializeKDTree(const string& filename) {
    ifstream ifs(filename);
    json j;
    ifs >> j;
    Node* root = deserializeNode(j);
    ifs.close();
    cout << "KD-Tree has been deserialized from " << filename << "\n";
    return root;
}
//user interface
void commandLineInterface() {
    Node* root = NULL;
    int choice;
    string fileName;

    while (true) {
        cout << "\nMenu:\n";
        cout << "1. Load cities from CSV file\n";
        cout << "2. Insert a new city\n";
        cout << "3. Insert multiple cities from a CSV file\n";
        cout << "4. Nearest neighbor search\n";
        cout << "5. Range search\n";
        cout << "6. Save KD-Tree to a file\n";
        cout << "7. Load KD-Tree from a file\n";
        cout << "8. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1:
                cout << "Enter the CSV file name: ";
                cin >> fileName;
                root = KDTree(fileName);
                cout << "Cities loaded from " << fileName << ".\n";
                break;

            case 2: {
                Data newCity;
                cout << "Enter city name: ";
                cin >> ws;  
                getline(cin, newCity.city);
                cout << "Enter latitude, longitude: ";
                cin >> newCity.lat >> newCity.lng;
                cout << "Enter name of the country: ";
                cin >> ws;
                getline(cin, newCity.country);
                
                cout << "Enter the population: ";
                cin >> newCity.pop;
                root = insertRec(root, newNode(newCity), 0);
                cout << "City " << newCity.city << " inserted.\n";
                break;
            }

            case 3:
                cout << "Enter the CSV file name: ";
                cin >> fileName;
                root = KDTree(fileName);
                cout << "Cities loaded from " << fileName << ".\n";
                break;

            case 4: {
                double lat, lng;
                cout << "Enter latitude and longitude for nearest neighbor search: ";
                cin >> lat >> lng;
                Node target;
                target.point.lat = lat;
                target.point.lng = lng;
                Node* nearest = nearestNeighbor(root, &target, 0);
                if (nearest) {
                    cout << "Nearest city is: " << nearest->point.city << " (" << nearest->point.lat << ", " << nearest->point.lng << "), "
                        << nearest->point.country << ", Population: " << nearest->point.pop << "\n";
                } else {
                    cout << "No cities found.\n";
                }
                vector<Data> results;
                results.push_back(nearest->point);
                string outputFile = "output.csv";
                saveToCSV(results, outputFile);
                cout << "Results saved to " << outputFile << ".\n";
                break;
                break;
            }

            case 5: {
                double minLat, minLng, maxLat, maxLng;
                cout << "Enter min latitude, min longitude, max latitude, max longitude for range search: ";
                cin >> minLat >> minLng >> maxLat >> maxLng;
                vector<Data> results = rangeSearch(root, minLat, minLng, maxLat, maxLng);
                cout << "Found " << results.size() << " cities within the specified range.\n";
                printResults(results);

                string outputFile = "output.csv";
                saveToCSV(results, outputFile);
                cout << "Results saved to " << outputFile << ".\n";
                break;
            }

            case 6: {
                cout << "Enter filename to save KD-Tree: ";
                cin >> fileName;
                serializeKDTree(root, fileName);
                break;
            }
            case 7: {
                cout << "Enter filename to load KD-Tree: ";
                cin >> fileName;
                root = deserializeKDTree(fileName);
                break;
            }
            case 8:
                cout << "Exiting...\n";
                return;
            default:
                cout << "Invalid choice. Please try again.\n";
        }
    }
}

int main() {
    commandLineInterface();
    return 0;
}