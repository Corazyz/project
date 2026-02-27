#include "stdio.h"
#include "math.h"
#include "random"
#include "unordered_map"

#define FLT_MAX 10000000.0

static inline void standard_euclidean(
    float*              std_euclidean_res,
    const float*        ref_data,
    const float*        Test_Input,
    const int           n_ref,
    const int           n_feats) {
    for (int i = 0; i < n_ref; i++) {
        // Compute distances from query to all data points
        float dist_sq = 0.0;
        for (int j = 0; j < n_feats; j++) {
            float diff = ref_data[i * n_feats + j] - Test_Input[j];
            dist_sq += diff * diff;
        }
        std_euclidean_res[i] = dist_sq;
    }
}

static void findNearest(
    const int       n_feats,
    const float*    sqdist,
    const int       n_ref,
    const int       k,
    float*          distance,
    int*            indices) {
    // Find k smallest distances using a simple approach
    // for better performance, consider using a priority queue
    for (int neighbor = 0; neighbor < k; neighbor++) {
        float min_dist = FLT_MAX;
        int min_index = -1;
        for (int i = 0; i < n_ref; i++) {
            bool already_selected = false;
            for (int j = 0; j < neighbor; j++) {
                if (indices[j] == i) {
                    already_selected = true;
                    break;
                }
            }
            if (already_selected) continue;

            // Find the minimum distance among remaining points
            if (sqdist[i] < min_dist) {
                min_dist = sqdist[i];
                min_index = i;
            }
        }
        if (min_index != -1) {
            // Store results
            distance[neighbor] = sqrtf(min_dist);
            indices[neighbor] = min_index;
        }
    }
}

static void get_prediction(int n_tests, int *indices, int k, const float* ref_labels, float *predictions) {
    // Classify each test point
    for (int i = 0; i < n_tests; i++) {
        std::unordered_map<int, int> label_counts;
        // Count occurrences of each label among neighbors
        for (int j = 0; j < k; j++) {
            int neighbor_index = indices[i * k + j];
            float label = ref_labels[neighbor_index];
            label_counts[label]++;
        }
        int max_count = 0;
        float predicted_label = -1.0;

        for(const auto& pair : label_counts) {
            if(pair.second > max_count || (pair.second == max_count && pair.first < predicted_label)) {
                max_count = pair.second;
                predicted_label = pair.first;
            }
        }
        predictions[i] = predicted_label;
    }
}
void _knn2_classifier(
    const float*        ref_data,       // Referenced data points(n_ref x n_features)
    const float*        ref_labels,     // Corresponding labels(n_ref)
    const float*        Test_input,     // test data points(n_tests x n_features)
    float*              distance,       // Output distances(n_tests x k)
    int*                indices,        // Output indices of nearest neighbors(n_tests x k)
    float*              predictions,    // Prediction index of test data(n_test)
    const int           n_tests,        // Number of test data
    const int           n_ref,          // Number of referenced data
    const int           n_feats,        // Number of features
    const int           k               // Number of neighbors
    ) {
    // Validate k is reasonable
    if (k < 0 || k > n_ref) {
        printf("[ERROR] k must be between 1 and number of ref data\n");
        return;
    }

    // Temporary storage for all distances
    float *all_distance = new float[n_ref];

    float *id_dist = new float[k];
    int *id_index = new int[k];

    // Compute distances from Test_input to all data points
    for (int i = 0; i < n_tests; i++) {
        standard_euclidean(all_distance, ref_data, Test_input + i * n_feats, n_ref, n_feats);
        findNearest(n_feats, all_distance, n_ref, k, id_dist, id_index);
        for (int j = 0; j < k; j++) {
            distance[i * k + j] = id_dist[j];
            indices[i * k + j] = id_index[j];
        }
    }

    get_prediction(n_tests, indices, k, ref_labels, predictions);


    delete[] all_distance;
    delete[] id_dist;
    delete[] id_index;
}

void generate_data(float *ref_data, float *test_data, int n_ref, int n_test, int n_feat, int n_class, float *ref_label, float *test_label) {
    // Set seed for random generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate n_class normal distributions
    std::vector<std::normal_distribution<>> distributions;
    for (int c = 0; c < n_class; c++) {
        // You can customize mean and stddev for each class here
        // For example, means spaced evenly between -2.0 and 2.0
        double mean = -2.0 + 4.0 * c / (n_class - 1);
        double stddev = 0.3 + 0.1 * c;  // Varying stddev slightly
        distributions.emplace_back(mean, stddev);
    }

    // Generate ref_data
    for (int i = 0; i < n_ref; i++) {
        // Assign class labels evenly
        int class_id = i % n_class;
        ref_label[i] = static_cast<float>(class_id);

        for (int j = 0; j < n_feat; j++) {
            // Generate data from the corresponding distribution
            ref_data[i * n_feat + j] = distributions[class_id](gen);
        }
    }

    // Generate test_data
    // Generate test_data with different proportions
    // Define the desired proportions for each class in test data
    std::vector<float> class_proportions(n_class);
    // Example: Class 0 gets 50%, Class 1 gets 30%, Class 2 gets 20% (for n_class=3)
    // You can customize these proportions as needed
    float sum = 0.0f;
    for (int c = 0; c < n_class; c++) {
        class_proportions[c] = 1.0f / (c + 1);  // Example decreasing proportion
        sum += class_proportions[c];
    }
    // Normalize proportions
    for (int c = 0; c < n_class; c++) {
        class_proportions[c] /= sum;
    }

    // Generate test samples according to proportions
    std::discrete_distribution<> test_dist(class_proportions.begin(), class_proportions.end());

    for (int i = 0; i < n_test; i++) {
        // Assign class labels according to specified proportions
        int class_id = test_dist(gen);
        test_label[i] = static_cast<float>(class_id);

        for (int j = 0; j < n_feat; j++) {
            // Generate data from the corresponding distribution
            test_data[i * n_feat + j] = distributions[class_id](gen);
        }
    }
}

int main() {
    // // Example data: 5 samples with 2 features
    // float data[] = {1.0, 1.0,
    //                 1.1, 1.1,
    //                 2.0, 2.0,
    //                 2.1, 2.1,
    //                 5.0, 5.0};
    // float labels[] = {0, 0, 1, 1, 2};
    // int shape[] = {5, 2};

    // // Query point
    // float query[] = {1.5, 1.5};

    // // Output arrays
    // float distances[3]; // for k = 3
    // int indices[3];

    // _knn2_classifier(data, labels, query, distances, indices, 1, 5, 2,);
    int n_ref = 256;
    int n_test = 100;
    int n_feat = 8;
    int n_class = 3;
    int k = 9;
    // Shape_data ref, test;
    // ref.row = n_ref;
    // ref.col = n_feat;
    // test.row = n_test;
    // test.col = n_feat;
    // gen_param(&param, n_ref, n_test, n_feat, k);
    float *ref_data = new float[n_ref * n_feat];
    float *ref_label = new float[n_ref];

    float *test_data = new float[n_test * n_feat];
    float *test_label = new float[n_test];

    generate_data(ref_data, test_data, n_ref, n_test, n_feat, n_class, ref_label, test_label);

    float *distance = new float[n_test * k];
    int *indices = new int[n_test * k];
    float *prediction = new float[n_test];
    _knn2_classifier(ref_data, ref_label, test_data, distance, indices, prediction, n_test, n_ref, n_feat, k);
    // Print results
    int class0_count = 0;
    int class1_count = 0;
    int class2_count = 0;
    for (int i = 0; i < n_test; i++) {
        if (prediction[i] == 0.0f) {
            class0_count++;
        } else if (prediction[i] == 1.0f){
            class1_count++;
        } else {
            class2_count++;
        }
    }
    printf("Classification Results:\n");
    printf("Test# \tPredicted Class\n");
    printf("----------------------\n");

    for (int i = 0; i < n_test; i++) {
        printf("%d\t%f\n", i, prediction[i]);
    }

    printf("\nClassification Summary:\n");
    printf("Class 0: %d samples (%.1f%%)\n", class0_count, (float)class0_count/n_test*100);
    printf("Class 1: %d samples (%.1f%%)\n", class1_count, (float)class1_count/n_test*100);
    printf("Class 2: %d samples (%.1f%%)\n", class2_count, (float)class2_count/n_test*100);
    return 0;
}

static void tpu_apply_win(u64 XR, u64 XI, int batch, int L, u64 window, bool real_input)
{
    int align_byte = tpu_eu_num(DT_FP32) * tpu_data_type_size(DT_FP32);
    int size_max = ALIGN(tpu_local_mem_size_per_npu() / 3, align_byte);
    local_addr_t win = 0;
    local_addr_t input0 = size_max;
    local_addr_t input1 = input0 + size_max;
    int num_per_c = size_max / (L * sizeof(float));
    int num_c = num_per_c * NPU_NUM;
    int i = 0;
    int j = 0;
    u64 global_xr, global_xi = 0;
    int loop = DIV_UP(batch, num_c);
    int k;

    if (num_c > batch) {
        num_c = batch;
    }

    for(k = 0; k < loop; ++k) {
        if (k == loop - 1) {
            num_c = batch - i;
        }
        dim4 src0_shape = {.n = 1, .c = 1, .h = 1, .w = L};
        dim4 src0_stride = {.n = NO_USE, .c = NO_USE, .h = NO_USE, .w = 1};
        dim4 src_shape = {.n = 1, .c = num_c, .h = 1, .w = L};
        dim4 src_stride = {.n = NO_USE, .c = L, .h = NO_USE, .w = 1};
        global_xr = XR + i * L * sizeof(DT_FP32);
        global_xi = XI + i * L * sizeof(DT_FP32);

        tpu_gdma_cpy_S2L(input0, global_xr, &src_shape, &src_stride, &src_stride, DT_FP32);
        if (!real_input) {
            tpu_gdma_cpy_S2L(input1, global_xi, &src_shape, &src_stride, &src_stride, DT_FP32);
        }
        tpu_poll();

        if (k == 0) {
            num_per_c = DIV_UP(num_c, NPU_NUM);
            tpu_gdma_cpy_S2L(win, window, &src0_shape, &src0_stride, &src0_stride, DT_FP32);
            tpu_poll();
            for (j = 0; j < num_per_c; ++j) {
                tpu_bdc_cpy(win + j * L * sizeof(DT_FP32), win, &src0_shape, &src0_stride, &src0_stride, DT_FP32);
            }
            dim4 cpy_shape = {.n = 1, .c = NPU_NUM, .h = 1, .w = L * num_per_c};
            tpu_bdc_npu_bcast(win, win, &cpy_shape, DT_FP32);

        }
        tpu_bdc_fp_mul(input0, input0, win, &src_shape, &src_stride, &src_stride, &src_stride, DT_FP32);
        if (!real_input) {
            tpu_bdc_fp_mul(input1, input1, win, &src_shape, &src_stride, &src_stride, &src_stride, DT_FP32);
        }
        tpu_poll();
        tpu_gdma_cpy_L2S(global_xr, input0, &src_shape, &src_stride, &src_stride, DT_FP32);
        if (!real_input) {
            tpu_gdma_cpy_L2S(global_xi, input1, &src_shape, &src_stride, &src_stride, DT_FP32);
        }
        tpu_poll();
        i += num_c;
    }
}