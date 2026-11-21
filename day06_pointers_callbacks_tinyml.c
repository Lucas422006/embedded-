/**
    * ============================================================================
    * BÀI HỌC & THỬ THÁCH EMBEDDED C - NGÀY 06 (Thời lượng: 60 phút - Độ khó: Trung bình)
    * Chủ đề: Số học Con trỏ (Pointer Arithmetic), Con trỏ Tổng quát (`void *`),
    *         Con trỏ Đầu ra (Output Pointers) & Con trỏ Hàm (Callback) cho TinyML
    * Tài liệu tra cứu chuẩn:
    *   - Con trỏ & Con trỏ hàm trong C: https://cppreference.net/c/language.html
    *   - Quy trình lưu code Git:        https://git-scm.com/cheat-sheet
    * ============================================================================
    *
    * ============================================================================
    * PHẦN 1: BÀI HỌC LÝ THUYẾT MỚI (15 - 20 PHÚT ĐỌC KỸ TRƯỚC KHI CODE)
    * ============================================================================
    *
    * 1. SỐ HỌC CON TRỎ (POINTER ARITHMETIC) — TĂNG TỐC VÒNG LẶP DSP & TINYML
    * ----------------------------------------------------------------------------
    * Khi bạn có một con trỏ `p` trỏ tới phần tử đầu tiên của một mảng:
    *   - Phép toán `p++` KHÔNG phải lúc nào cũng cộng thêm 1 byte vào địa chỉ!
    *   - Nó tự động nhảy đúng bằng `sizeof(kiểu_dữ_liệu)` byte:
    *       + `int8_t  *p1`: `p1++` nhảy +1 byte.
    *       + `int16_t *p2`: `p2++` nhảy +2 byte.
    *       + `int32_t *p3`: `p3++` nhảy +4 byte.
    *   - Biểu thức `*p++` (rất hay gặp trong nhân CMSIS-NN / LiteRT):
    *     Bước 1: Đọc giá trị tại địa chỉ hiện tại (`*p`).
    *     Bước 2: Sau khi đọc xong, dịch con trỏ `p` sang phần tử kế tiếp.
    *
    * 2. TRẢ VỀ NHIỀU KẾT QUẢ BẰNG CON TRỎ ĐẦU RA (OUTPUT POINTERS) & CHỐNG TRÀN SỐ
    * ----------------------------------------------------------------------------
    * Trong C, một hàm chỉ `return` được 1 giá trị duy nhất. Muốn 1 hàm trả về đồng
    * thời 3 thông số (ví dụ: `min`, `max`, `mean` của một cửa sổ tín hiệu), ta truyền
 * địa chỉ của 3 biến hứng kết quả vào hàm:
 *     void Calc(const int16_t *arr, size_t len, int16_t *out_min, int16_t *out_max);
 * - Từ khóa `const int16_t *arr`: Cam kết hàm CHỈ ĐỌC mảng đầu vào, cấm sửa.
 * - Lưu ý chống tràn số (Overflow): Khi cộng dồn nhiều số `int16_t` (tối đa 32.767)
 *   hoặc nhân 2 số `int8_t` (120 * 120 = 14.400), biến cộng dồn BẮT BUỘC phải là
 *   `int32_t` (Accumulator) rồi sau cùng mới chia hoặc ép kiểu!
 *
 * 3. CON TRỎ TỔNG QUÁT (`void *`) — XỬ LÝ BỘ NHỚ KHÔNG PHÂN BIỆT KIỂU DỮ LIỆU
 * ----------------------------------------------------------------------------
 * `void *` là con trỏ có thể chứa địa chỉ của BẤT KỲ kiểu dữ liệu nào (`uint16_t`,
 * `uint32_t`, `float`, `struct`...). Tuy nhiên, CPU không biết `void *` rộng bao
 * nhiêu byte nên bạn KHÔNG thể viết `*vptr` hay `vptr++` trực tiếp.
 * -> Cách dùng chuẩn trong Firmware: Ép kiểu sang `uint8_t *` để thao tác từng byte:
 *       uint8_t *byte_ptr = (uint8_t *)buf;
 *
 * 4. CON TRỎ HÀM (FUNCTION POINTER & CALLBACK) — VŨ KHÍ TỐI THƯỢNG CỦA KỸ SƯ NHÚNG
 * ----------------------------------------------------------------------------
 * Không chỉ biến mới có địa chỉ trong RAM, mà chính HÀM (Function) cũng nằm ở
 * một địa chỉ trong bộ nhớ Flash. Ta có thể lưu địa chỉ của một hàm vào biến
 * gọi là **Con trỏ hàm** để truyền hàm này vào làm tham số cho hàm khác!
 *
 * Ví dụ định nghĩa kiểu con trỏ hàm kích hoạt mạng nơ-ron (Activation Function):
 *     // Kiểu `ActivationFn_t` trỏ tới bất kỳ hàm nào nhận `int32_t` và trả về `int8_t`
 *     typedef int8_t (*ActivationFn_t)(int32_t x);
 *
 * Khi viết hàm chạy mạng nơ-ron:
 *     bool RunLayer(..., ActivationFn_t act_fn) {
 *         int8_t y = act_fn(acc); // Gọi hàm thông qua con trỏ hàm!
 *     }
 * Lúc chạy thực tế, bạn muốn lớp mạng dùng hàm `ReLU` thì truyền tên hàm `Act_ReLU_INT8`,
 * muốn đổi sang hàm kẹp biên thì truyền `Act_Clamp_INT8` mà KHÔNG phải sửa code của `RunLayer`!
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * PHẦN 2: 4 THỬ THÁCH NÂNG CAO NGÀY 06 (45 PHÚT TỰ SUY NGHĨ & CODE)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * THỬ THÁCH 1: TÍNH TÍCH VÔ HƯỚNG (DOT PRODUCT) TINYML BẰNG SỐ HỌC CON TRỎ
 * Đặc tả kỹ thuật:
 *   - Đây là phép toán cốt lõi nhất của mọi mạng nơ-ron (MAC: Multiply-Accumulate):
 *         sum = inputs[0]*weights[0] + inputs[1]*weights[1] + ... + inputs[len-1]*weights[len-1]
 *   - Nếu `inputs == NULL` HOẶC `weights == NULL` HOẶC `len == 0`: trả về `0`.
 *   - Yêu cầu nâng cao: KHÔNG sử dụng toán tử mảng `[i]`. Hãy duyệt hoàn toàn bằng
 *     số học con trỏ (`*inputs++`, `*weights++`) và cộng dồn vào biến `int32_t`.
 * ---------------------------------------------------------------------------- */
int32_t TinyML_DotProduct_INT8(const int8_t *inputs, const int8_t *weights, size_t len) {
    // TODO 1: Tính tích vô hướng int8_t * int8_t vào biến tích lũy int32_t dùng số học con trỏ
    if( inputs == NULL || weights == NULL || len == 0 ) return 0;
    int n = 0;
    int sum = 0;
    while( n < len ){
        sum += (*inputs)*(*weights);
        *inputs++;
        *weights++;
        n++;
    }
    return sum;
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 2: TÌM MIN, MAX, TRUNG BÌNH QUA CON TRỎ ĐẦU RA & CHỐNG TRÀN SỐ
 * Đặc tả kỹ thuật:
 *   - Nếu `data == NULL` HOẶC `len == 0` HOẶC bất kỳ con trỏ đầu ra nào (`out_min`,
 *     `out_max`, `out_mean`) bằng `NULL`: trả về `false`.
 *   - Duyệt mảng `data` gồm `len` phần tử kiểu `int16_t`:
 *       + Tìm giá trị nhỏ nhất ghi vào `*out_min`.
 *       + Tìm giá trị lớn nhất ghi vào `*out_max`.
 *       + Tính tổng bằng biến `int32_t` (tránh tràn `int16_t`!) rồi chia cho `(int32_t)len`,
 *         ép kiểu về `int16_t` và ghi vào `*out_mean`.
 *   - Trả về `true`.
 * ---------------------------------------------------------------------------- */
bool Signal_FindMinMaxMean(const int16_t *data,
                           size_t len,
                           int16_t *out_min,
                           int16_t *out_max,
                           int16_t *out_mean) {
    // TODO 2: Tính Min, Max, Mean (dùng tổng int32_t) và trả kết quả qua 3 con trỏ đầu ra
    if( data == NULL || len == 0 || out_max == NULL || out_mean == NULL || out_min == NULL) return false;
    
    *out_min = 32767;
    *out_max = data[0];
    *out_mean = 0;
    int32_t sum =0;
    for(int i = 0; i < len; i++){
        if( *out_min >= data[i]){
            *out_min = data[i]; 
        } 
        if( *out_max <= data[i]){
            *out_max = data[i];
        } 
         sum += data[i] ;
        *out_mean = (int16_t)(sum/(int32_t)len);
    }
    
    return true;
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 3: ĐẢO NGƯỢC THỨ TỰ BYTE TẠI CHỖ VỚI CON TRỎ TỔNG QUÁT `void *`
 * Đặc tả kỹ thuật (Ứng dụng chuyển đổi Big-Endian <-> Little-Endian cho mọi kiểu dữ liệu):
 *   - Nếu `buf == NULL` HOẶC `byte_len == 0`: trả về `false`.
 *   - Ép kiểu `buf` sang con trỏ `uint8_t *`.
 *   - Dùng kỹ thuật 2 con trỏ:
 *       + `left` trỏ vào byte đầu tiên (`0`).
 *       + `right` trỏ vào byte cuối cùng (`byte_len - 1`).
 *     Trong khi `left < right`, hoán đổi (swap) giá trị `*left` và `*right`,
 *     sau đó tăng `left++` và giảm `right--`.
 *   - Trả về `true`.
 * ---------------------------------------------------------------------------- */
bool Mem_ReverseBytes(void *buf, size_t byte_len) {
    // TODO 3: Ép kiểu void* -> uint8_t* và đảo ngược mảng byte tại chỗ bằng 2 con trỏ
    if( buf == NULL || byte_len == 0) return false;
    
    uint8_t *byte = (uint8_t *) buf;
    
    uint8_t *left = byte;
    uint8_t *right = byte + byte_len - 1;
    while( left < right){
        uint8_t phan_tu = *left;
        *left = *right;
        *right = phan_tu;
        *left++;
        *right--;
        
    }

    return true;
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 4: CON TRỎ HÀM (CALLBACK) — LỚP KÍCH HOẠT MẠNG NƠ-RON LINH HOẠT
 * Đặc tả kỹ thuật:
 *   - Bước A: Đã khai báo sẵn kiểu con trỏ hàm `ActivationFn_t` ở dưới.
 *   - Bước B: Hoàn thiện 2 hàm kích hoạt rời rạc:
 *       1. `Act_ReLU_INT8(int32_t x)`:
 *          + Nếu `x < 0`   -> trả về `0`.
 *          + Nếu `x > 127` -> trả về `127`.
 *          + Ngược lại     -> trả về `(int8_t)x`.
 *       2. `Act_Clamp_INT8(int32_t x)`:
 *          + Nếu `x < -128` -> trả về `-128`.
 *          + Nếu `x > 127`  -> trả về `127`.
 *          + Ngược lại      -> trả về `(int8_t)x`.
 *   - Bước C: Hoàn thiện hàm `TinyML_ApplyActivation`:
 *       + Nếu `in_acc == NULL` HOẶC `out_q7 == NULL` HOẶC `act_fn == NULL` HOẶC `len == 0`:
 *         trả về `false`.
 *       + Duyệt qua `len` phần tử, gọi hàm thông qua con trỏ `act_fn(in_acc[i])` và lưu
 *         kết quả vào `out_q7[i]`.
 *       + Trả về `true`.
 * ---------------------------------------------------------------------------- */
typedef int8_t (*ActivationFn_t)(int32_t x);

int8_t Act_ReLU_INT8(int32_t x) {
    // TODO 4A: Cài đặt hàm kích hoạt ReLU giới hạn biên [0 .. 127]
    if( x < 0 ) return 0;
    else if( x > 127) return 127;
    else return (int8_t)x;
}

int8_t Act_Clamp_INT8(int32_t x) {
    // TODO 4B: Cài đặt hàm kẹp biên bão hòa [-128 .. 127]
    if( x < -128 ) return -128;
    else if( x > 127) return 127;
    else return (int8_t)x;
    return 0;
}

bool TinyML_ApplyActivation(const int32_t *in_acc,
                            int8_t *out_q7,
                            size_t len,
                            ActivationFn_t act_fn) {
    // TODO 4C: Kiểm tra tham số và áp dụng callback `act_fn` cho toàn bộ mảng đầu vào
    if( in_acc == NULL || out_q7 == NULL || act_fn == NULL || len == 0) return false;
    for (int i = 0; i < len ; i ++){
        out_q7[i] = act_fn(in_acc[i]);
    }
    return true;
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 06 (KHÔNG SỬA ĐỔI HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;

    printf("=== BAT DAU CHAM DIEM THU THACH NGAY 06 (CON TRO & CALLBACK TINYML) ===\n\n");

    /* --- Test Thử thách 1: Dot Product bằng Pointer Arithmetic --- */
    const int8_t vec_in[4]  = {10, -20, 30, -40};
    const int8_t vec_w[4]   = {3, -3, 4, 5};
    // 10*3 + (-20)*(-3) + 30*4 + (-40)*5 = 30 + 60 + 120 - 200 = 10
    int32_t dot1 = TinyML_DotProduct_INT8(vec_in, vec_w, 4U);

    const int8_t big_in[2]  = {120, 120};
    const int8_t big_w[2]   = {120, 120};
    // 14400 + 14400 = 28800 (kiểm tra tích lũy bằng int32_t)
    int32_t dot2 = TinyML_DotProduct_INT8(big_in, big_w, 2U);
    int32_t dot_null = TinyML_DotProduct_INT8(NULL, vec_w, 4U);

    if ((dot1 == 10) && (dot2 == 28800) && (dot_null == 0)) {
        printf("[PASS] Thu thach 1: TinyML_DotProduct_INT8 tinh tich vo huong chinh xac (dot1=%d, dot2=%d)!\n",
               (int)dot1, (int)dot2);
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 1: dot1=%d (ky vong 10), dot2=%d (ky vong 28800), null=%d\n",
               (int)dot1, (int)dot2, (int)dot_null);
    }

    /* --- Test Thử thách 2: Output Pointers & Chống tràn tổng 16-bit --- */
    // Tổng = 20000 - 15000 + 30000 + 10000 + 5000 = 50000 (Tràn int16_t nếu không dùng int32_t!)
    const int16_t sig[5] = {20000, -15000, 30000, 10000, 5000};
    int16_t s_min = 0, s_max = 0, s_mean = 0;
    bool stat_ok = Signal_FindMinMaxMean(sig, 5U, &s_min, &s_max, &s_mean);
    bool stat_null = Signal_FindMinMaxMean(sig, 5U, NULL, &s_max, &s_mean);

    if ((stat_ok == true) && (stat_null == false) &&
        (s_min == -15000) && (s_max == 30000) && (s_mean == 10000)) {
        printf("[PASS] Thu thach 2: Signal_FindMinMaxMean (min=%d, max=%d, mean=%d) khong tran so!\n",
               s_min, s_max, s_mean);
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 2: ok=%d, min=%d(-15000), max=%d(30000), mean=%d(10000)\n",
               stat_ok, s_min, s_max, s_mean);
    }

    /* --- Test Thử thách 3: Con trỏ void* đảo ngược byte tại chỗ --- */
    uint8_t buf_even[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    uint8_t buf_odd[5]  = {1U, 2U, 3U, 4U, 5U};
    bool rev1 = Mem_ReverseBytes(buf_even, sizeof(buf_even));
    bool rev2 = Mem_ReverseBytes(buf_odd, sizeof(buf_odd));
    bool rev_null = Mem_ReverseBytes(NULL, 4U);

    bool even_match = (buf_even[0] == 0x78U) && (buf_even[1] == 0x56U) &&
                      (buf_even[2] == 0x34U) && (buf_even[3] == 0x12U);
    bool odd_match  = (buf_odd[0] == 5U) && (buf_odd[1] == 4U) && (buf_odd[2] == 3U) &&
                      (buf_odd[3] == 2U) && (buf_odd[4] == 1U);

    if (rev1 && rev2 && !rev_null && even_match && odd_match) {
        printf("[PASS] Thu thach 3: Mem_ReverseBytes xu ly void* cho mang chan & le chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 3: even=[0x%02X 0x%02X 0x%02X 0x%02X], odd=[%u %u %u %u %u]\n",
               buf_even[0], buf_even[1], buf_even[2], buf_even[3],
               buf_odd[0], buf_odd[1], buf_odd[2], buf_odd[3], buf_odd[4]);
    }

    /* --- Test Thử thách 4: Con trỏ hàm Callback Activation --- */
    const int32_t acc_in[4] = {-200, -15, 64, 300};
    int8_t out_relu[4]  = {0};
    int8_t out_clamp[4] = {0};

    bool cb1 = TinyML_ApplyActivation(acc_in, out_relu, 4U, Act_ReLU_INT8);
    bool cb2 = TinyML_ApplyActivation(acc_in, out_clamp, 4U, Act_Clamp_INT8);
    bool cb_null = TinyML_ApplyActivation(acc_in, out_relu, 4U, NULL);

    bool relu_ok  = (out_relu[0] == 0) && (out_relu[1] == 0) &&
                    (out_relu[2] == 64) && (out_relu[3] == 127);
    bool clamp_ok = (out_clamp[0] == -128) && (out_clamp[1] == -15) &&
                    (out_clamp[2] == 64) && (out_clamp[3] == 127);

    if (cb1 && cb2 && !cb_null && relu_ok && clamp_ok) {
        printf("[PASS] Thu thach 4: TinyML_ApplyActivation goi Callback ReLU & Clamp chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 4: relu=[%d %d %d %d] (ky vong 0 0 64 127), clamp=[%d %d %d %d] (ky vong -128 -15 64 127)\n",
               out_relu[0], out_relu[1], out_relu[2], out_relu[3],
               out_clamp[0], out_clamp[1], out_clamp[2], out_clamp[3]);
    }

    printf("\n=== KET QUA TONG KET NGAY 06: %u / 4 THU THACH DAT [PASS] ===\n", pass_count);
    return 0;
}