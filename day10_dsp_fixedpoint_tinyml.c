/**
 * ============================================================================
 * BÀI HỌC & BÀI TẬP EMBEDDED C - NGÀY 10 (Thời lượng: 60 phút - Độ khó: Trung bình)
 * Chủ đề: Lọc Số Thời Gian Thực (Moving Average, IIR/EMA) & Toán Học Fixed-Point
 *         Cho Tiền Xử Lý Dữ Liệu Cảm Biến Embedded AI
 * Tài liệu tra cứu chuẩn:
 *   - Chuẩn ngôn ngữ C:       https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git: https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: CƠ CHẾ HOẠT ĐỘNG CỐT LÕI (KHÔNG CÓ CÔNG THỨC SẴN — TỰ SUY LUẬN)
 * ============================================================================
 *
 * 1. BỘ LỌC TRUNG BÌNH TRƯỢT TỐI ƯU O(1) (MOVING AVERAGE FILTER)
 *    - Để khử nhiễu gai trên tín hiệu cảm biến trước khi đưa vào mô hình AI, ta
 *      lấy trung bình cộng của `N` mẫu gần nhất (cửa sổ trượt).
 *    - Nếu mỗi lần có 1 mẫu mới lại chạy vòng lặp cộng lại cả `N` phần tử thì
 *      tốn `O(N)` phép cộng (rất chậm khi `N` lớn, ví dụ 128 hoặc 256 mẫu).
 *    - Cơ chế tối ưu `O(1)`: Duy trì một bộ đệm vòng chứa đúng `N` mẫu và một
 *      biến tích lũy `running_sum`. Khi có mẫu mới đến, chỉ cần trừ đi mẫu cũ
 *      nhất sắp bị đẩy ra khỏi cửa sổ và cộng thêm mẫu mới vào `running_sum`
 *      (chú ý biến tổng phải đủ rộng để không tràn số khi `N` lớn).
 *
 * 2. BỘ LỌC THÔNG THẤP IIR / EMA BẰNG SỐ NGUYÊN (EXPONENTIAL MOVING AVERAGE)
 *    - Thay vì lưu mảng `N` mẫu (tốn RAM), bộ lọc EMA chỉ lưu đúng 1 trạng thái
 *      đầu ra gần nhất `prev_y`. Đầu ra mới là sự pha trộn có trọng số giữa mẫu
 *      thô mới `x` và giá trị cũ `prev_y` theo hệ số làm mượt `alpha` (từ 0 đến 1).
 *    - Trên vi điều khiển không có bộ tính số thực (FPU) như STM32F103, dùng
 *      `float` cực kỳ chậm. Kỹ sư nhúng chuyển hệ số pha trộn thành phân số có
 *      mẫu số là lũy thừa của 2 (ví dụ chia 16 hoặc 256) để tính hoàn toàn bằng
 *      số nguyên và phép dịch bit, kèm cộng thêm nửa mẫu số để làm tròn đúng.
 *
 * 3. TOÁN HỌC DẤU PHẨY TĨNH (FIXED-POINT Q15 / INT8 QUANTIZATION)
 *    - Mô hình TinyML lượng tử hóa làm việc với kiểu `int8_t` (miền `[-128, 127]`)
 *      thay vì `float`.
 *    - Phép nhân hai số Q-format tạo ra kết quả có độ rộng bit gấp đôi và phần
 *      thập phân bị dịch đi gấp đôi, nên bắt buộc phải tính trên thanh ghi trung
 *      gian rộng hơn (như `int32_t`), sau đó dịch phải về đúng tỷ lệ gốc và kẹp
 *      biên bão hòa (Saturation) để tránh tràn ngược dấu.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================================
 * PHẦN 2: 4 BÀI TẬP THỰC HÀNH (TỰ THIẾT KẾ STRUCT, THUẬT TOÁN & CÔNG THỨC)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * BÀI TẬP 1: BỘ LỌC TRUNG BÌNH TRƯỢT O(1) KHÔNG TRÀN SỐ (MOVING AVERAGE)
 * Yêu cầu hành vi:
 *   - Tự thiết kế các trường bên trong `MovingAvg_t` (xóa biến `_replace_me`).
 *   - Bộ lọc sử dụng mảng `buf` (kiểu `int16_t`) có kích thước `win_size` do bên
 *     ngoài cấp phát tĩnh.
 *   - `MA_Init`: Khởi tạo bộ lọc về trạng thái rỗng (xóa sạch bộ đệm, `win_size > 0`).
 *   - `MA_Update`: Đưa 1 mẫu mới `sample` vào bộ lọc và trả về giá trị trung bình
 *     hiện tại (kiểu `int16_t`).
 *       + Trong giai đoạn làm đầy (số mẫu đã nạp < `win_size`), chia cho số mẫu
 *         thực tế đã nhận được tới thời điểm đó (ví dụ mẫu thứ 1 chia 1, mẫu thứ
 *         2 chia 2...).
 *       + Khi đã đủ `win_size` mẫu, luôn cập nhật tổng và đầu ra trong thời gian
 *         hằng số O(1) (KHÔNG dùng vòng lặp `for` cộng lại cả mảng mỗi lần gọi!).
 *       + Xử lý an toàn khi con trỏ NULL (trả 0).
 * ---------------------------------------------------------------------------- */
typedef struct {
    /* TODO 1A: Xóa biến tạm và tự thiết kế các trường quản lý bộ lọc Moving Average */
    size_t win_size;
    int16_t *buf;
    int64_t running_sum;
    size_t count;
    size_t buf_idx;
} MovingAvg_t;

bool MA_Init(MovingAvg_t *ma, int16_t *buf, size_t win_size) {
    // TODO 1B
    if( ma == NULL || buf == NULL || win_size == 0) return false;
    ma->win_size = win_size;
    ma->buf = buf;
    ma->buf_idx = 0;
    ma->running_sum = 0;
    for( size_t i = 0; i < ma->win_size; i++){
        ma->buf[i] = 0;
    }
    
    ma->count = 0;
    return true;
}

int16_t MA_Update(MovingAvg_t *ma, int16_t sample) {
    // TODO 1C
    if( ma == NULL || ma->buf == NULL || ma->win_size == 0) return 0;
    if( ma->count < ma->win_size ){
        ma->buf[ma->buf_idx++] = sample;
        if( ma->buf_idx == ma->win_size) ma->buf_idx = 0;
        ma->running_sum += sample;
        ma->count++;
        return  (int16_t)((ma->running_sum)/(int32_t)(ma->count));
    } else if( ma->count == ma->win_size){
        ma->running_sum -= ma->buf[ma->buf_idx];
        ma->buf[ma->buf_idx] = sample;
        ma->buf_idx++;
        if( ma->buf_idx == ma->win_size) ma->buf_idx = 0 ;
        ma->running_sum += sample;
        return (int16_t)((ma->running_sum)/(int32_t)(ma->count));
    }
    return 0;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 2: BỘ LỌC THÔNG THẤP IIR / EMA BẰNG SỐ NGUYÊN (KHÔNG DÙNG FLOAT)
 * Yêu cầu hành vi:
 *   - Bộ lọc làm mượt theo công thức pha trộn:
 *         y_new = alpha * x + (1 - alpha) * y_prev
 *     trong đó hệ số `alpha` được biểu diễn dưới dạng số nguyên `alpha_q8`
 *     thuộc đoạn `[1 .. 255]` tương ứng với `alpha = alpha_q8 / 256.0`.
 *   - Không được dùng `float` hay `double` ở bất kỳ dòng nào!
 *   - `EMA_Init`: Khởi tạo bộ lọc với `alpha_q8` (từ chối nếu `alpha_q8 == 0`
 *     hoặc `ema == NULL`), đánh dấu chưa có mẫu đầu tiên.
 *   - `EMA_Update`:
 *       + Ngay tại mẫu ĐẦU TIÊN sau khi `Init`: không pha trộn với giá trị 0,
 *         mà gán thẳng đầu ra bằng chính `sample` đó để tránh trễ khởi động.
 *       + Từ mẫu thứ 2 trở đi: tính giá trị mới bằng số nguyên, có làm tròn về
 *         số nguyên gần nhất (Round-Half-Up cho số dương, hoặc làm tròn đối xứng
 *         cho cả số âm/dương).
 * ---------------------------------------------------------------------------- */
typedef struct {
    /* TODO 2A: Xóa biến tạm và tự thiết kế các trường cho bộ lọc EMA số nguyên */
    int8_t alpha_8 ;
    int16_t y_prev;
    bool initialized;
} IntEMA_t;

bool EMA_Init(IntEMA_t *ema, uint8_t alpha_q8) {
    // TODO 2B
    if( alpha_q8 == 0 || ema == NULL ) return  false;
    ema->alpha_8 = alpha_q8;
    ema->y_prev = 0;
    ema->initialized = false;
    return true;
}

int16_t EMA_Update(IntEMA_t *ema, int16_t sample) {
    // TODO 2C
    if( ema == NULL ) return 0;
    if( ema->initialized == false){
        ema->initialized = true;
        ema->y_prev = sample;
    } else{
        uint32_t y_new = (int32_t)(ema->alpha_8* sample) + (int32_t)(256 - ema->alpha_8)*ema->y_prev;
        if( y_new > 0){
            ema->y_prev = (uint32_t)(y_new + 128)/256;
        } else if( y_new < 0 ){
            ema->y_prev = (int32_t)(y_new-128)/256;
        }
    }
    return ema->y_prev;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 3: LƯỢNG TỬ HÓA ĐẦU VÀO CẢM BIẾN SANG `int8_t` CHO TINYML
 * Yêu cầu hành vi:
 *   - Một mô hình TinyML nhận đầu vào kiểu `int8_t` trong miền `[-128, 127]`.
 *   - Ta cần ánh xạ tuyến tính một giá trị đo cảm biến `raw_val` (thuộc dải
 *     `[in_min, in_max]` với `in_max > in_min`) sang miền `[-127, 127]` theo
 *     tỷ lệ đối xứng quanh tâm của dải đo (tâm dải đo ứng với `0`, `in_max` ứng
 *     với `+127`, `in_min` ứng với `-127`).
 *   - Yêu cầu:
 *       + Tính hoàn toàn bằng số nguyên (tuyệt đối không dùng `float`/`double`).
 *       + Nếu `raw_val` vượt ra ngoài `[in_min, in_max]`, kẹp chặt đầu ra vào
 *         biên `[-127, 127]`.
 *       + Nếu `in_max <= in_min`, trả về `0`.
 * ---------------------------------------------------------------------------- */
int8_t TinyML_QuantizeSample(int16_t raw_val, int16_t in_min, int16_t in_max) {
    // TODO 3
    if( in_min >= in_max ) return 0;
    if( raw_val >= in_max) return 127;
    if( raw_val <= in_min ) return -127;
    int32_t span = in_max - in_min;
    int32_t offset = raw_val - in_min;
    return (offset * 254 + span/2)/span - 127;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 4: TRÍCH XUẤT ĐẶC TRƯNG NĂNG LƯỢNG & ĐỘ BIẾN THIÊN TÍN HIỆU (DSP -> AI)
 * Yêu cầu hành vi:
 *   - Để nhận diện rung động bất thường của động cơ, thay vì đưa cả mảng thô
 *     vào mạng nơ-ron, ta trích xuất 2 đặc trưng miền thời gian từ mảng `samples`:
 *       1. `out_mean_abs`: Trung bình giá trị tuyệt đối (Mean Absolute Value - MAV)
 *          của toàn bộ `len` mẫu (phản ánh năng lượng/biên độ trung bình).
 *       2. `out_zcr`: Số lần đổi dấu (Zero-Crossing Rate - ZCR) giữa 2 mẫu liên
 *          tiếp (phản ánh tần số dao động). Quy ước một cặp `(samples[i], samples[i+1])`
 *          tính là 1 lần đổi dấu nếu một số DƯƠNG HẲN (`> 0`) và số kia ÂM HẲN (`< 0`).
 *          (Các mẫu bằng `0` không làm phát sinh giao cắt).
 *   - Trả về `false` nếu có con trỏ `NULL` hoặc `len == 0`, ngược lại trả `true`.
 * ---------------------------------------------------------------------------- */
bool DSP_ExtractFeatures(const int16_t *samples,
                         size_t len,
                         uint16_t *out_mean_abs,
                         uint16_t *out_zcr) {
    // TODO 4
    uint32_t sum = 0;
    if( samples == NULL || out_mean_abs == NULL || out_zcr == NULL || len == 0) return false;
    for( size_t i = 0; i < len; i++){
        sum += abs(samples[i]);
    }
    uint16_t count = 0;
    *out_mean_abs = sum/len;
    if( len <  2) *out_zcr = 0;
    else {
    for( size_t i = 0; i + 1  < len ; i++){
        if( (samples[i] < 0 && samples[i + 1] > 0) || (samples[i] > 0 && samples[i+ 1] < 0) ){
            count++;
        }
    }}
    *out_zcr = count;
    return true;
}


/* ============================================================================
 * HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 10 (4 BÀI TẬP - KHÔNG SỬA HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0U;

    printf("=== BAT DAU CHAM DIEM BAI TAP NGAY 10 (LOC SO DSP & FIXED-POINT) ===\n\n");

    /* --- Test Bài tập 1: Moving Average O(1) --- */
    int16_t ma_buf[4] = {0};
    MovingAvg_t ma;
    bool t1 = MA_Init(&ma, ma_buf, 4U) && !MA_Init(&ma, NULL, 4U) && !MA_Init(&ma, ma_buf, 0U);

    // Nạp lần lượt: 100 -> TB=100; 200 -> TB=150; 300 -> TB=200; 400 -> TB=250; 500 -> cửa sổ [200,300,400,500] -> TB=350
    int16_t m1 = MA_Update(&ma, 100);
    int16_t m2 = MA_Update(&ma, 200);
    int16_t m3 = MA_Update(&ma, 300);
    int16_t m4 = MA_Update(&ma, 400);
    int16_t m5 = MA_Update(&ma, 500);

    // Kiểm tra chống tràn int16_t khi nạp 4 mẫu lớn (30000 * 4 = 120000 > 32767)
    int16_t big_buf[4] = {0};
    MovingAvg_t ma_big;
    MA_Init(&ma_big, big_buf, 4U);
    MA_Update(&ma_big, 30000);
    MA_Update(&ma_big, 30000);
    MA_Update(&ma_big, 30000);
    int16_t m_big = MA_Update(&ma_big, 30000);

    if (t1 && (m1 == 100) && (m2 == 150) && (m3 == 200) && (m4 == 250) && (m5 == 350) && (m_big == 30000)) {
        printf("[PASS] Bai tap 1: Moving Average O(1) va chong tran so chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 1: m=[%d, %d, %d, %d, %d] (100,150,200,250,350), m_big=%d (30000)\n",
               m1, m2, m3, m4, m5, m_big);
    }

    /* --- Test Bài tập 2: Bộ lọc EMA số nguyên (alpha_q8 = 64 => 64/256 = 0.25) --- */
    IntEMA_t ema;
    bool t2 = EMA_Init(&ema, 64U) && !EMA_Init(&ema, 0U) && !EMA_Init(NULL, 64U);
    int16_t e1 = EMA_Update(&ema, 1000); // Mẫu đầu tiên -> phải trả thẳng 1000
    int16_t e2 = EMA_Update(&ema, 1400); // 0.25*1400 + 0.75*1000 = 350 + 750 = 1100
    int16_t e3 = EMA_Update(&ema, 1400); // 0.25*1400 + 0.75*1100 = 350 + 825 = 1175

    // Kiểm tra làm tròn khi chia không chẵn: 0.25*1002 + 0.75*1000 = 1000.5 -> làm tròn thành 1001
    IntEMA_t ema_round;
    EMA_Init(&ema_round, 64U);
    EMA_Update(&ema_round, 1000);
    int16_t er = EMA_Update(&ema_round, 1002);

    if (t2 && (e1 == 1000) && (e2 == 1100) && (e3 == 1175) && (er == 1001)) {
        printf("[PASS] Bai tap 2: IntEMA so nguyen va lam tron chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 2: e1=%d(1000), e2=%d(1100), e3=%d(1175), er=%d(1001)\n",
               e1, e2, e3, er);
    }

    /* --- Test Bài tập 3: Lượng tử hóa int8_t --- */
    // Dải đo [0 .. 1000] -> tâm = 500 -> 0; 1000 -> +127; 0 -> -127
    int8_t q_mid   = TinyML_QuantizeSample(500, 0, 1000);
    int8_t q_max   = TinyML_QuantizeSample(1000, 0, 1000);
    int8_t q_min   = TinyML_QuantizeSample(0, 0, 1000);
    int8_t q_clamp_hi = TinyML_QuantizeSample(2500, 0, 1000);
    int8_t q_clamp_lo = TinyML_QuantizeSample(-500, 0, 1000);
    // Dải đối xứng [-2000 .. 2000], mẫu = 1000 -> khoảng +63 hoặc +64
    int8_t q_half  = TinyML_QuantizeSample(1000, -2000, 2000);

    if ((q_mid == 0) && (q_max == 127) && (q_min == -127) &&
        (q_clamp_hi == 127) && (q_clamp_lo == -127) &&
        (q_half >= 63) && (q_half <= 64)) {
        printf("[PASS] Bai tap 3: TinyML_QuantizeSample anh xa & kep bien int8_t chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 3: mid=%d(0), max=%d(127), min=%d(-127), hi=%d, lo=%d, half=%d\n",
               q_mid, q_max, q_min, q_clamp_hi, q_clamp_lo, q_half);
    }

    /* --- Test Bài tập 4: Trích xuất đặc trưng MAV & Zero-Crossing Rate --- */
    const int16_t vib[6] = {100, -200, 0, -300, 400, -100};
    // |x| = {100, 200, 0, 300, 400, 100} -> tổng = 1100 -> MAV = 1100 / 6 = 183
    // Cặp đổi dấu trực tiếp (>0 và <0): (100, -200), (-300, 400), (400, -100) => 3 lần
    uint16_t mav = 0U, zcr = 0U;
    bool f_ok = DSP_ExtractFeatures(vib, 6U, &mav, &zcr);
    bool f_bad = DSP_ExtractFeatures(NULL, 6U, &mav, &zcr);

    if (f_ok && !f_bad && (mav == 183U) && (zcr == 3U)) {
        printf("[PASS] Bai tap 4: DSP_ExtractFeatures tinh MAV=%u va ZCR=%u chinh xac!\n", mav, zcr);
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 4: f_ok=%d, mav=%u (ky vong 183), zcr=%u (ky vong 3)\n", f_ok, mav, zcr);
    }

    printf("\n=== KET QUA TONG KET NGAY 10: %u / 4 BAI TAP DAT [PASS] ===\n", pass_count);
    return 0;
}