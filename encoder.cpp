#include <iostream>
#include <vector>
#include <fstream>
#include <bitset>

using namespace std;

class Encoder
{
private:
    vector<int> output;
    vector<string> dictionary;
    vector<char> input;

    // Initialize dictionary with all single-character strings
    void initializeDictionary()
    {
        dictionary.clear();

        for (int i = 0; i < 256; i++)
        {
            string s(1, (char)i);
            dictionary.push_back(s);
        }
    }

    // Find dictionary index
    int findInDictionary(const string &str)
    {
        for (int i = 0; i < dictionary.size(); i++)
        {
            if (dictionary[i] == str)
            {
                return i;
            }
        }

        return -1;
    }

public:
    Encoder()
    {
        initializeDictionary();
    }

    // Manual text input
    void editInput(const string &text)
    {
        input.clear();

        for (char c : text)
        {
            input.push_back(c);
        }
    }

    // File input (text or image)
    bool loadFile(const string &filename)
    {
        input.clear();

        ifstream file(filename, ios::binary);

        if (!file)
        {
            cout << "Failed to open file.\n";
            return false;
        }

        char byte;

        while (file.get(byte))
        {
            input.push_back(byte);
        }

        file.close();

        return true;
    }

    // Encode function
    vector<int> encode()
    {
        output.clear();

        if (input.empty())
        {
            return output;
        }

        string W = "";

        int i = 0;

        while (i < input.size())
        {
            W += input[i];

            // Find longest matching string
            while ((i + 1 < input.size()) &&
                   (findInDictionary(W + input[i + 1]) != -1))
            {
                i++;
                W += input[i];
            }

            // Emit dictionary index
            int index = findInDictionary(W);

            if (index != -1)
            {
                output.push_back(index);
            }

            // Add W + next symbol to dictionary
            if (i + 1 < input.size())
            {
                string newEntry = W + input[i + 1];

                if (findInDictionary(newEntry) == -1)
                {
                    dictionary.push_back(newEntry);
                }
            }

            i++;
            W = "";
        }

        return output;
    }

    // Print compressed output indices
    void printOutput()
    {
        cout << "\nCompressed Output Indices:\n";

        for (int code : output)
        {
            cout << code << " ";
        }

        cout << endl;
    }

    // Print original message in binary
    void printOriginalBinary()
    {
        cout << "\nOriginal Message in Binary:\n";

        for (char c : input)
        {
            bitset<8> binary((unsigned char)c);
            cout << binary << " ";
        }

        cout << endl;
    }

    // Print compressed message in binary
    void printCompressedBinary()
    {
        cout << "\nCompressed Message in Binary:\n";

        for (int code : output)
        {
            // 12-bit representation commonly used in LZW
            bitset<12> binary(code);
            cout << binary << " ";
        }

        cout << endl;
    }

    // Optional dictionary print
    void printDictionary()
    {
        cout << "\nDictionary:\n";

        for (int i = 0; i < dictionary.size(); i++)
        {
            cout << i << " : ";

            for (char c : dictionary[i])
            {
                if (isprint(c))
                {
                    cout << c;
                }
                else
                {
                    cout << "[" << (int)(unsigned char)c << "]";
                }
            }

            cout << endl;
        }
    }
};


// --------------------------------------------------
// Temporary Main Function
// --------------------------------------------------

int main()
{
    Encoder encoder;

    int choice;

    cout << "===== Encoder Test Program =====\n";
    cout << "1. Manual Text Input\n";
    cout << "2. File Input (Text/Image)\n";
    cout << "Enter choice: ";

    cin >> choice;
    cin.ignore();

    if (choice == 1)
    {
        string text;

        cout << "\nEnter text to encode:\n";
        getline(cin, text);

        encoder.editInput(text);
    }
    else if (choice == 2)
    {
        string filename;

        cout << "\nEnter file name/path:\n";
        getline(cin, filename);

        if (!encoder.loadFile(filename))
        {
            return 1;
        }
    }
    else
    {
        cout << "Invalid choice.\n";
        return 1;
    }

    // Encode data
    encoder.encode();

    // Display outputs
    encoder.printOriginalBinary();

    encoder.printOutput();

    encoder.printCompressedBinary();

    // Optional:
    // encoder.printDictionary();

    return 0;
}