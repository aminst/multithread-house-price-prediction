#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <chrono>

constexpr int COLUMN_COUNT = 9;
constexpr int SALES_PRICE_INDEX = 8;
constexpr char CSV_DELIM = ',';

constexpr int NORM_INDEX_ZERO = 0;
constexpr int STD_INDEX_ZERO = 1;
constexpr int NORM_INDEX_ONE = 2;
constexpr int STD_INDEX_ONE = 3;

constexpr int GrLivArea_INDEX = 5;

using namespace std;



vector<vector<int>> read_dataset(ifstream &in_file)
{
    string line;
    vector<vector<int>> dataset;

    getline(in_file, line); // header of file

    while (getline(in_file, line, '\n'))
    {
        vector<int> result;
        stringstream ss(line);
        string str;
        while (getline(ss, str, CSV_DELIM))
            result.push_back(stoi(str));
        dataset.push_back(result);
    }
    return dataset;
}

vector<vector<int>> convert_sale_prices(vector<vector<int>> dataset, int threshold)
{
    vector<vector<int>> changed_dataset;
    for (size_t i = 0; i < dataset.size(); i++)
    {
        changed_dataset.push_back(vector<int>());
        for (size_t j = 0; j < dataset[i].size(); j++)
        {
            if (j != SALES_PRICE_INDEX)
                changed_dataset[i].push_back(dataset[i][j]);
            else
            {
                if (dataset[i][SALES_PRICE_INDEX] < threshold)
                    changed_dataset[i].push_back(0);
                else
                    changed_dataset[i].push_back(1);
            }
        }
    }
    return changed_dataset;
}

vector<vector<double>> calculate_norm_std(vector<vector<int>> dataset)
{
    vector<vector<int>> dataset_zero;
    vector<vector<int>> dataset_one;

    for (size_t i = 0; i < dataset.size(); i++)
    {
        if (dataset[i][SALES_PRICE_INDEX] == 0)
            dataset_zero.push_back(dataset[i]);
        else
            dataset_one.push_back(dataset[i]);
    }

    vector<vector<double>> norm_std(4);
    for (size_t i = 0; i < COLUMN_COUNT - 1; i++)
    {
        double sum = 0;
        double sum_square = 0;
        for (size_t j = 0; j < dataset_zero.size(); j++)
        {
            for (size_t t = 0; t < dataset_zero[j].size(); t++)
            {
                if (t == i)
                {
                    sum += dataset_zero[j][t];
                    sum_square += pow(dataset_zero[j][t], 2);
                }
            }
        }
        double mean = sum / dataset_zero.size();
        double stdev = sqrt((sum_square / dataset_zero.size()) - pow(mean, 2));

        norm_std[NORM_INDEX_ZERO].push_back(mean);
        norm_std[STD_INDEX_ZERO].push_back(stdev);
    }

    for (size_t i = 0; i < COLUMN_COUNT - 1; i++)
    {
        double sum = 0;
        double sum_square = 0;
        for (size_t j = 0; j < dataset_one.size(); j++)
        {
            for (size_t t = 0; t < dataset_one[j].size(); t++)
            {
                if (t == i)
                {
                    sum += dataset_one[j][t];
                    sum_square += pow(dataset_one[j][t], 2);
                }
            }
        }
        double mean = sum / dataset_one.size();
        double stdev = sqrt((sum_square / dataset_one.size()) - pow(mean, 2));

        norm_std[NORM_INDEX_ONE].push_back(mean);
        norm_std[STD_INDEX_ONE].push_back(stdev);
    }
    return norm_std;
}

vector<int> predict(vector<vector<int>> dataset, double mean_grliv_one, double std_grliv_one)
{
    double low = mean_grliv_one - std_grliv_one;
    double high = mean_grliv_one + std_grliv_one;
    vector<int> prediction;
    for (size_t i = 0; i < dataset.size(); i++)
    {
        if (dataset[i][GrLivArea_INDEX] > low && dataset[i][GrLivArea_INDEX] < high)
            prediction.push_back(1);
        else
            prediction.push_back(0);
    }
    return prediction;
}

double get_accuraccy(vector<vector<int>> dataset, vector<int> prediction)
{
    int correct_count = 0;
    for (size_t i = 0; i < dataset.size(); i++)
    {
        if(dataset[i][SALES_PRICE_INDEX] == prediction[i])
            correct_count++;
    }
    return ((double)correct_count)/((double)dataset.size()) * 100;
}

int main(int argc, char* argv[])
{
    auto start = chrono::high_resolution_clock::now();

    ifstream dataset_stream;

    const int DATASET_DIR_INDEX = 1;
    const int PRICE_THRESHOLD_INDEX = 2;
    dataset_stream.open(string(argv[DATASET_DIR_INDEX]) + "/dataset.csv");
    
    vector<vector<int>> dataset = read_dataset(dataset_stream);
    vector<vector<int>> changed = convert_sale_prices(dataset, stoi(argv[PRICE_THRESHOLD_INDEX]));
    vector<vector<double>> norm_std = calculate_norm_std(changed);
    vector<int> prediction = predict(changed, norm_std[NORM_INDEX_ONE][GrLivArea_INDEX], norm_std[STD_INDEX_ONE][GrLivArea_INDEX]);

    double acc = get_accuraccy(changed, prediction);
    printf("Accuracy: %.2f%%\n", acc);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    // cout << "Time taken by function: "
    //     << (double)duration.count() / (double)1000000 << " seconds" << endl;


    return 0;
}