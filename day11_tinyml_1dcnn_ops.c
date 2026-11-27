/**
 * ============================================================================
 * BÀI HỌC & BÀI TẬP EMBEDDED C - NGÀY 11 (Thời lượng: 60 phút - Độ khó: Trung bình)
 * Chủ đề: Các Toán Tử Cốt Lõi Của Mạng Tích Chập 1 Chiều (1D-CNN) Cho TinyML
 *         (Conv1D INT8, MaxPooling 1D, ArgMax Ngưỡng Tự Tin & Pipeline Đệm Tĩnh)
 * Tài liệu tra cứu chuẩn:
 *   - Chuẩn ngôn ngữ C:       https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git: https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: CƠ CHẾ HOẠT ĐỘNG CỐT LÕI (KHÔNG CÓ CÔNG THỨC SẴN — TỰ SUY LUẬN)
 * ============================================================================
 *
 * 1. TÍCH CHẬP 1 CHIỀU (1D CONVOLUTION — VALID, STRIDE = 1)
 *    - Khác với lớp Dense (mỗi nơ-ron nhìn toàn bộ mảng đầu vào), lớp Conv1D
 *      dùng một bộ lọc ngắn gọi là `kernel` (dài `K` phần tử) trượt dần từng
 *      bước qua chuỗi tín hiệu cảm biến dài `N` phần tử (như sóng rung động,
 *      âm thanh, nhịp tim hoặc dữ liệu IMU).
 *    - Tại mỗi vị trí trượt mà `kernel` nằm gọn hoàn toàn bên trong mảng đầu vào
 *      (chế độ "Valid", không đệm 0 ở hai đầu), ta tính tổng nhân tích lũy giữa
 *      đoạn tín hiệu đó với `kernel`, cộng thêm độ lệch `bias`, đưa về đúng tỷ
 *      lệ lượng tử hóa bằng dịch phải, rồi kẹp biên bão hòa về kiểu `int8_t`.
 *
 * 2. GỘP CỰC ĐẠI 1 CHIỀU (1D MAX POOLING)
 *    - Sau khi tích chập tạo ra bản đồ đặc trưng (feature map), ta chia mảng đó
 *      thành các khối nhỏ liên tiếp không chồng lấn có kích thước `pool_size`
 *      (nếu cuối mảng còn thừa ít hơn `pool_size` phần tử thì bỏ qua phần dư).
 *    - Với mỗi khối, ta chỉ giữ lại GIÁ TRỊ LỚN NHẤT. Cơ chế này giúp giảm mạnh
 *      số lượng phép tính cho các lớp phía sau và giúp mô hình ít nhạy cảm hơn
 *      khi tín hiệu cảm biến bị dịch lệch nhẹ theo thời gian.
 *
 * 3. QUYẾT ĐỊNH NHÃN PHÂN LOẠI (ARGMAX KÈM NGƯỠNG TỰ TIN)
 *    - Đầu ra cuối cùng của mô hình phân loại là một mảng điểm số (logits) ứng
 *      với từng nhãn (ví dụ: 0 = Bình thường, 1 = Lệch trục, 2 = Hỏng vòng bi).
 *    - Trong sản phẩm thực tế, nếu tín hiệu nhiễu làm mọi lớp đều có điểm thấp,
 *      hệ thống không được đoán bừa lớp cao nhất mà phải so sánh điểm cao nhất
 *      với một ngưỡng tự tin tối thiểu (`min_confidence`); dưới ngưỡng thì trả
 *      về trạng thái "Không xác định" (`-1`).
 *
 * 4. PIPELINE DÙNG BỘ ĐỆM TẠM TĨNH (SCRATCH BUFFER — KHÔNG DÙNG MALLOC)
 *    - Trên vi điều khiển, ta nối nhiều lớp mạng liên tiếp bằng cách truyền vào
 *      một vùng nhớ tạm (`scratch_buf`) đã cấp phát tĩnh sẵn để chứa đầu ra trung
 *      gian của lớp trước làm đầu vào cho lớp sau (tương tự `tensor_arena`).
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * PHẦN 2: 4 BÀI TẬP THỰC HÀNH (KHÔNG GỢI Ý CÔNG THỨC — TỰ THIẾT KẾ THUẬT TOÁN)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * BÀI TẬP 1: LỚP TÍCH CHẬP 1 CHIỀU (1D CONVOLUTION — VALID, STRIDE = 1)
 * Yêu cầu hành vi:
 *   - Thực hiện tích chập 1D chế độ Valid giữa mảng `input` (độ dài `in_len`)
 *     và bộ lọc `kernel` (độ dài `kernel_len`), lưu kết quả vào `output`.
 *   - Với mỗi vị trí đầu ra:
 *       + Cộng dồn tích vô hướng vào biến tích lũy đủ rộng tránh tràn số,
 *         sau đó cộng thêm `bias`.
 *       + Dịch phải số học đi `right_shift` bit.
 *       + Kẹp biên bão hòa (Clamp) vào miền `int8_t` (`[-128, 127]`).
 *   - Xử lý an toàn: Nếu con trỏ NULL, `kernel_len == 0`, hoặc `in_len < kernel_len`:
 *     trả về `0`.
 *   - Giá trị trả về: Tổng số phần tử đầu ra hợp lệ đã ghi vào `output`.
 * ---------------------------------------------------------------------------- */
size_t TinyML_Conv1D_Valid_INT8(const int8_t *input,
                                size_t in_len,
                                const int8_t *kernel,
                                size_t kernel_len,
                                int32_t bias,
                                uint8_t right_shift,
                                int8_t *output) {
    // TODO 1
    if( input == NULL || kernel == NULL || output == NULL || kernel_len == 0 || in_len < kernel_len ) return 0;
    
    for( size_t i = 0; i < in_len - kernel_len + 1; i++){
        int64_t sum = 0;
        for( size_t k = 0; k < kernel_len; k++){
        sum += (int64_t)input[i + k]*kernel[k];
    }
        sum += bias;
        sum >>= right_shift;
        if( sum >= 127) sum = 127;
    else if( sum <= -128) sum = -128;
        output[i] = (int8_t)sum;
    }


    return in_len - kernel_len + 1;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 2: LỚP GỘP CỰC ĐẠI 1 CHIỀU (1D MAX POOLING)
 * Yêu cầu hành vi:
 *   - Chia mảng `input` (độ dài `in_len`) thành các khối liên tiếp không chồng
 *     lấn có kích thước `pool_size` (bỏ qua phần dư ở cuối nếu không đủ 1 khối).
 *   - Ghi giá trị lớn nhất của từng khối vào mảng `output`.
 *   - Lưu ý tình huống biên: Toàn bộ phần tử trong một khối đều có thể là SỐ ÂM!
 *   - Nếu con trỏ NULL, `pool_size == 0`, hoặc `in_len < pool_size`: trả về `0`.
 *   - Giá trị trả về: Số lượng khối (phần tử đầu ra) đã ghi vào `output`.
 * ---------------------------------------------------------------------------- */
size_t TinyML_MaxPool1D_INT8(const int8_t *input,
                             size_t in_len,
                             size_t pool_size,
                             int8_t *output) {
    // TODO 2
    if( input == NULL || in_len == 0 || pool_size == 0 || output == NULL || in_len < pool_size ) return false;
    size_t count = 0;
    for( size_t i = 0; i < in_len; i += pool_size){
        int8_t max = -128;
        if( i + pool_size > in_len){
            for( size_t k = i; k < in_len; k++){
            if( max < input[k]){
                max = input[k];
            }
            output[count] = max;
        }
        } else {
        for( size_t k = i; k < i + pool_size; k++){
            if( max < input[k]){
                max = input[k];
            }
        }
        output[count] = max;
            count++;
    }
    }
    return count;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 3: TÌM NHÃN DỰ ĐOÁN CAO NHẤT KÈM NGƯỠNG TỰ TIN (ARGMAX)
 * Yêu cầu hành vi:
 *   - Duyệt mảng điểm số `logits` (độ dài `num_classes`) để tìm chỉ số của phần
 *     tử có giá trị lớn nhất.
 *   - Nếu có nhiều lớp bằng điểm nhau và cùng đạt cực đại: ưu tiên trả về chỉ
 *     số NHỎ NHẤT (xuất hiện đầu tiên).
 *   - Nếu giá trị lớn nhất tìm được NHỎ HƠN HẲN `min_confidence`: trả về `-1`.
 *   - Nếu `logits == NULL` hoặc `num_classes == 0`: trả về `-1`.
 * ---------------------------------------------------------------------------- */
int16_t TinyML_ArgMax_INT8(const int8_t *logits,
                           size_t num_classes,
                           int8_t min_confidence) {
    // TODO 3
    if( logits == NULL || num_classes == 0 ) return -1;
    size_t addr = 0;
    int16_t max = -256;
    for( size_t i = 0; i < num_classes; i++){
        if( max < logits[i]) {
            max = logits[i];
            addr = i;
        }
    }
    if( max < min_confidence) return -1;
    return addr;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 4: GHÉP NỐI PIPELINE 1D-CNN MINI (CONV1D -> RELU -> MAXPOOL -> ARGMAX)
 * Yêu cầu hành vi:
 *   - Xây dựng hàm chạy nối tiếp trọn bộ 4 công đoạn của một mạng 1D-CNN siêu nhẹ
 *     mà KHÔNG dùng `malloc`:
 *       1. Kiểm tra tham số hợp lệ và kiểm tra xem dung lượng bộ đệm tạm
 *          `scratch_cap` có đủ chứa toàn bộ đầu ra của bước Conv1D hay không
 *          (nếu thiếu dung lượng hoặc tham số sai -> trả về `-1`).
 *       2. Chạy bước tích chập 1D (dùng lại hàm Bài 1), lưu đầu ra vào `scratch_buf`.
 *       3. Áp dụng hàm kích hoạt ReLU ngay tại chỗ trên `scratch_buf` (mọi phần
 *          tử âm bị đưa về `0`, phần tử không âm giữ nguyên).
 *       4. Chạy bước MaxPool1D (dùng lại hàm Bài 2) với kích thước `pool_size`,
 *          lưu kết quả rút gọn ngay vào đầu mảng `scratch_buf` (hoặc vùng hợp lệ
 *          của `scratch_buf`).
 *       5. Chạy bước ArgMax (dùng lại hàm Bài 3) trên mảng kết quả sau MaxPool
 *          với ngưỡng `min_confidence` và trả về chỉ số nhãn (hoặc `-1`).
 * ---------------------------------------------------------------------------- */
int16_t TinyML_RunMiniCNN(const int8_t *input,
                          size_t in_len,
                          const int8_t *kernel,
                          size_t kernel_len,
                          int32_t bias,
                          uint8_t right_shift,
                          size_t pool_size,
                          int8_t min_confidence,
                          int8_t *scratch_buf,
                          size_t scratch_cap) {
    // TODO 4
    if( in_len == 0 || input == NULL || kernel == NULL || scratch_buf == NULL || kernel_len == 0 ||  in_len < kernel_len || pool_size == 0 || scratch_cap < in_len - kernel_len + 1) return -1;
    TinyML_Conv1D_Valid_INT8( input, in_len, kernel,kernel_len,bias, right_shift, scratch_buf);
    for( size_t i = 0; i < in_len - kernel_len + 1; i++){
        if( scratch_buf[i] < 0) scratch_buf[i] = 0;
    }
    size_t x = in_len - kernel_len + 1;
    TinyML_MaxPool1D_INT8(scratch_buf,x,pool_size,scratch_buf);
    return TinyML_ArgMax_INT8(scratch_buf,x,min_confidence);
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 11 (4 BÀI TẬP - KHÔNG SỬA HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0U;

    printf("=== BAT DAU CHAM DIEM BAI TAP NGAY 11 (1D-CNN TINYML) ===\n\n");

    /* --- Test Bài tập 1: Conv1D Valid INT8 --- */
    const int8_t sig1[5] = {10, 20, -10, 30, 40};
    const int8_t ker1[3] = {2, -1, 3};
    int8_t out1[5] = {0};
    size_t n1 = TinyML_Conv1D_Valid_INT8(sig1, 5U, ker1, 3U, 10, 1U, out1);

    // Kiểm tra kẹp biên bão hòa (+127)
    const int8_t sig_hi[3] = {100, 100, 100};
    const int8_t ker_hi[3] = {10, 10, 10};
    int8_t out_hi[1] = {0};
    size_t n_hi = TinyML_Conv1D_Valid_INT8(sig_hi, 3U, ker_hi, 3U, 0, 2U, out_hi);
    size_t n_bad = TinyML_Conv1D_Valid_INT8(sig1, 2U, ker1, 3U, 0, 0U, out1); // in_len < kernel_len

    if ((n1 == 3U) && (out1[0] == -10) && (out1[1] == 75) && (out1[2] == 40) &&
        (n_hi == 1U) && (out_hi[0] == 127) && (n_bad == 0U)) {
        printf("[PASS] Bai tap 1: Conv1D_Valid_INT8 tinh tich chap & kep bien chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 1: n1=%zu(3), out1=[%d %d %d] (-10 75 40), out_hi=%d(127), n_bad=%zu(0)\n",
               n1, out1[0], out1[1], out1[2], out_hi[0], n_bad);
    }

    /* --- Test Bài tập 2: MaxPool1D INT8 --- */
    const int8_t mp_in1[6] = {-10, 75, 40, -5, 12, 8};
    int8_t mp_out1[3] = {0};
    size_t p1 = TinyML_MaxPool1D_INT8(mp_in1, 6U, 2U, mp_out1);

    // Kiểm tra khối toàn số âm và bỏ phần dư cuối mảng (5 / 2 = 2 khối)
    const int8_t mp_in2[5] = {-50, -20, -80, 10, 99};
    int8_t mp_out2[2] = {0};
    size_t p2 = TinyML_MaxPool1D_INT8(mp_in2, 5U, 2U, mp_out2);

    if ((p1 == 3U) && (mp_out1[0] == 75) && (mp_out1[1] == 40) && (mp_out1[2] == 12) &&
        (p2 == 2U) && (mp_out2[0] == -20) && (mp_out2[1] == 10)) {
        printf("[PASS] Bai tap 2: MaxPool1D_INT8 xu ly so am va bo phan du chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 2: p1=%zu(3), out1=[%d %d %d], p2=%zu(2), out2=[%d %d] (-20 10)\n",
               p1, mp_out1[0], mp_out1[1], mp_out1[2], p2, mp_out2[0], mp_out2[1]);
    }

    /* --- Test Bài tập 3: ArgMax với ngưỡng tự tin --- */
    const int8_t lg1[4] = {-20, 45, 92, 92}; // Trùng max -> lấy chỉ số đầu tiên (2)
    const int8_t lg2[3] = {-50, -10, -30};   // Toàn số âm, max = -10 >= -20 -> chỉ số 1
    const int8_t lg3[3] = {10, 25, 15};      // Max = 25 < 40 -> trả -1

    int16_t a1 = TinyML_ArgMax_INT8(lg1, 4U, 50);
    int16_t a2 = TinyML_ArgMax_INT8(lg2, 3U, -20);
    int16_t a3 = TinyML_ArgMax_INT8(lg3, 3U, 40);
    int16_t a_null = TinyML_ArgMax_INT8(NULL, 3U, 0);

    if ((a1 == 2) && (a2 == 1) && (a3 == -1) && (a_null == -1)) {
        printf("[PASS] Bai tap 3: TinyML_ArgMax_INT8 tim nhan va loc nguong tu tin chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 3: a1=%d(2), a2=%d(1), a3=%d(-1), a_null=%d(-1)\n", a1, a2, a3, a_null);
    }

    /* --- Test Bài tập 4: Pipeline 1D-CNN hoàn chỉnh dùng scratch_buf tĩnh --- */
    const int8_t cnn_in[6] = {10, 20, -10, 30, 40, -20};
    int8_t scratch[8] = {0};
    // Conv1D -> [-10, 75, 40, -15] -> ReLU -> [0, 75, 40, 0] -> MaxPool(2) -> [75, 40] -> ArgMax -> 0
    int16_t cls_ok   = TinyML_RunMiniCNN(cnn_in, 6U, ker1, 3U, 10, 1U, 2U, 30, scratch, sizeof(scratch));
    int16_t cls_low  = TinyML_RunMiniCNN(cnn_in, 6U, ker1, 3U, 10, 1U, 2U, 80, scratch, sizeof(scratch));
    int16_t cls_oom  = TinyML_RunMiniCNN(cnn_in, 6U, ker1, 3U, 10, 1U, 2U, 30, scratch, 2U); // scratch quá nhỏ

    if ((cls_ok == 0) && (cls_low == -1) && (cls_oom == -1)) {
        printf("[PASS] Bai tap 4: Pipeline Mini 1D-CNN (Conv->ReLU->Pool->ArgMax) chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 4: cls_ok=%d(0), cls_low=%d(-1), cls_oom=%d(-1)\n", cls_ok, cls_low, cls_oom);
    }

    printf("\n=== KET QUA TONG KET NGAY 11: %u / 4 BAI TAP DAT [PASS] ===\n", pass_count);
    return 0;
}