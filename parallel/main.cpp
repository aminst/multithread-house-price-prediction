#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <pthread.h>
#include <map>
#include <chrono>

constexpr int COLUMN_COUNT = 9;
constexpr int SALES_PRICE_INDEX = 8;
constexpr char CSV_DELIM = ',';

constexpr int SUM_INDEX_ZERO = 0;
constexpr int SQUARE_SUM_INDEX_ZERO = 1;
constexpr int COUNT_INDEX_ZERO = 2;
constexpr int SUM_INDEX_ONE = 3;
constexpr int SQUARE_SUM_INDEX_ONE = 4;
constexpr int COUNT_INDEX_ONE = 5;

constexpr int NORM_INDEX_ZERO = 0;
constexpr int STD_INDEX_ZERO = 1;
constexpr int NORM_INDEX_ONE = 2;
constexpr int STD_INDEX_ONE = 3;


constexpr int GrLivArea_INDEX = 5;

constexpr int THREAD_COUNT = 4;

using namespace std;

string dataset_path;
int price_threshold;
map<int, vector<vector<int>>> datasets;

map<int, vector<vector<double>>> norm_std_all;
vector<vector<double>> norm_std_agg(4);

map<int, vector<int>> corrects;

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

int get_correct_count(vector<vector<int>> dataset, vector<int> prediction)
{
    int correct_count = 0;
    for (size_t i = 0; i < dataset.size(); i++)
    {
        if(dataset[i][SALES_PRICE_INDEX] == prediction[i])
            correct_count++;
    }
    return correct_count;
}

void calculate_zero_measures(vector<vector<int>> reversed_dataset, long thread_id)
{
    for (size_t i = 0; i < reversed_dataset.size() - 1; i++)
    {
        double sum = accumulate(reversed_dataset[i].begin(), reversed_dataset[i].end(), 0.0);
        double square_sum = 0;
        for (size_t j = 0; j < reversed_dataset[i].size(); j++)
            square_sum+= pow(reversed_dataset[i][j], 2);
        double count = reversed_dataset[i].size();

        norm_std_all[thread_id][SUM_INDEX_ZERO].push_back(sum);
        norm_std_all[thread_id][SQUARE_SUM_INDEX_ZERO].push_back(square_sum);
        norm_std_all[thread_id][COUNT_INDEX_ZERO].push_back(count);
    }
}

void calculate_one_measures(vector<vector<int>> reversed_dataset, long thread_id)
{
    for (size_t i = 0; i < reversed_dataset.size() - 1; i++)
    {
        double sum = accumulate(reversed_dataset[i].begin(), reversed_dataset[i].end(), 0.0);
        double square_sum = 0;
        for (size_t j = 0; j < reversed_dataset[i].size(); j++)
            square_sum+= pow(reversed_dataset[i][j], 2);
        double count = reversed_dataset[i].size();

        norm_std_all[thread_id][SUM_INDEX_ONE].push_back(sum);
        norm_std_all[thread_id][SQUARE_SUM_INDEX_ONE].push_back(square_sum);
        norm_std_all[thread_id][COUNT_INDEX_ONE].push_back(count);
    }
}

void* read_norm_std_th(void* id) {
    long thread_id = (long) id;
    
    ifstream dataset_stream;
    dataset_stream.open(dataset_path + "/dataset_" + to_string(thread_id) + ".csv");
    vector<vector<int>> dataset = read_dataset(dataset_stream);
    vector<vector<int>> changed = convert_sale_prices(dataset, price_threshold);
    datasets[thread_id] = changed;

    vector<vector<int>> reversed_dataset_zero(COLUMN_COUNT);
    vector<vector<int>> reversed_dataset_one(COLUMN_COUNT);

    for (size_t i = 0; i < changed.size(); i++)
    {
        for (size_t j = 0; j < changed[i].size(); j++)
        {
            if (changed[i][SALES_PRICE_INDEX] == 0)
                reversed_dataset_zero[j].push_back(changed[i][j]);
            else
                reversed_dataset_one[j].push_back(changed[i][j]);
        }
    }
    norm_std_all[thread_id] = vector<vector<double>>(6);

    calculate_zero_measures(reversed_dataset_zero, thread_id);
    calculate_one_measures(reversed_dataset_one, thread_id);
    
    pthread_exit(id);
}

void aggregate_zero_norm_std()
{
    double sum;
    double sum_square;
    double count;

    for (size_t i = 0; i < COLUMN_COUNT - 1; i++)
    {
        sum = 0;
        sum_square = 0;
        count = 0;
        for (size_t j = 0; j < THREAD_COUNT; j++)
        {
            sum += norm_std_all[j][SUM_INDEX_ZERO][i];
            sum_square += norm_std_all[j][SQUARE_SUM_INDEX_ZERO][i];
            count += norm_std_all[j][COUNT_INDEX_ZERO][i];
        }
        double norm = sum / count;
        double stdev = sqrt((sum_square / count) - pow(norm, 2));
        norm_std_agg[NORM_INDEX_ZERO].push_back(norm);
        norm_std_agg[STD_INDEX_ZERO].push_back(stdev);
    }
}

void aggregate_one_norm_std()
{
    double sum;
    double sum_square;
    double count;

    for (size_t i = 0; i < COLUMN_COUNT - 1; i++)
    {
        sum = 0;
        sum_square = 0;
        count = 0;
        for (size_t j = 0; j < THREAD_COUNT; j++)
        {
            sum += norm_std_all[j][SUM_INDEX_ONE][i];
            sum_square += norm_std_all[j][SQUARE_SUM_INDEX_ONE][i];
            count += norm_std_all[j][COUNT_INDEX_ONE][i];
        }
        double norm = sum / count;
        double stdev = sqrt((sum_square / count) - pow(norm, 2));
        norm_std_agg[NORM_INDEX_ONE].push_back(norm);
        norm_std_agg[STD_INDEX_ONE].push_back(stdev);
    }
}

void aggregate_norm_std()
{
    aggregate_zero_norm_std();
    aggregate_one_norm_std();
}

void* predict_th(void* id) {
    long thread_id = (long) id;
    map<int, vector<int>> preds;
    int correct = 0;
    int total = 0;

    preds[thread_id] = predict(datasets[thread_id], norm_std_agg[NORM_INDEX_ONE][GrLivArea_INDEX], norm_std_agg[STD_INDEX_ONE][GrLivArea_INDEX]);
    correct += get_correct_count(datasets[thread_id], preds[thread_id]);
    total += datasets[thread_id].size();

    vector<int> res;
    res.push_back(correct);
    res.push_back(total);
    corrects[thread_id] = res;

    pthread_exit(id);
}

int main(int argc, char* argv[])
{
    auto start = chrono::high_resolution_clock::now();
    ifstream dataset_stream;

    const int DATASET_DIR_INDEX = 1;
    const int PRICE_THRESHOLD_INDEX = 2;

    dataset_path = string(argv[DATASET_DIR_INDEX]);
    price_threshold = stoi(argv[PRICE_THRESHOLD_INDEX]);

    pthread_t thread[THREAD_COUNT];
    for (long id = 0; id < THREAD_COUNT; id++) {
		pthread_create(&thread[id], NULL, read_norm_std_th, (void*)id);
	}

	for(long id = 0; id < THREAD_COUNT; id++) {
		pthread_join(thread[id], NULL);

	}

    aggregate_norm_std();

    for (long id = 0; id < THREAD_COUNT; id++) {
		pthread_create(&thread[id], NULL, predict_th, (void*)id);
	}

    int correct_count = 0;
    int total_count = 0;
    for(long id = 0; id < THREAD_COUNT; id++) {
		pthread_join(thread[id], NULL);
        correct_count += corrects[id][0];
        total_count += corrects[id][1];
	}
    double acc = ((double)correct_count)/((double)total_count) * 100;
    printf("Accuracy: %.2f%%\n", acc);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    // cout << "Time taken by function: "
    //     << (double)duration.count() / (double)1000000 << " seconds" << endl;

    return 0;
}