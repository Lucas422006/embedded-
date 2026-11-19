/**
 * ============================================================================
 * BÀI HỌC & THỬ THÁCH EMBEDDED C - NGÀY 04 (Thời lượng: 60 phút)
 * Chủ đề: Từ khóa Bộ nhớ (volatile, const, static), Thanh ghi Write-1-to-Clear
 *         và Cơ chế phối hợp Ngắt (ISR) - Vòng lặp chính (Main Loop)
 * Tài liệu tra cứu chuẩn:
 *   - Từ khóa kiểu dữ liệu C (volatile/const): https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git hàng ngày:        https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: BÀI HỌC LÝ THUYẾT CỐT LÕI (15 PHÚT ĐỌC HIỂU TRƯỚC KHI CODE)
 * ============================================================================
 *
 * 1. TỪ KHÓA `volatile` — TẠI SAO BẮT BUỘC PHẢI CÓ TRONG LẬP TRÌNH NHÚNG?
 * ----------------------------------------------------------------------------
 * Khi bạn biên dịch code C với chế độ tối ưu hóa (ví dụ `-O2` trên STM32CubeIDE),
 * trình biên dịch rất thông minh: Nếu nó thấy trong hàm `main()` không có dòng nào
 * sửa biến `flag`, nó sẽ chỉ đọc `flag` từ RAM đúng 1 lần vào thanh ghi CPU rồi
 * dùng lại mãi mãi.
 *
 * Nhưng trong vi điều khiển, có 2 thứ có thể tự thay đổi giá trị bất ngờ mà hàm
 * `main()` không hề hay biết:
 *   (A) Thanh ghi phần cứng (Hardware Registers): Tự đổi bit khi có tín hiệu điện.
 *   (B) Biến dùng chung với hàm Ngắt (ISR): Ngắt có thể nhảy vào sửa biến bất cứ lúc nào.
 *
 * -> Giải pháp: Đặt từ khóa `volatile` trước biến/thanh ghi đó (ví dụ: `volatile bool flag;`).
 *    `volatile` ra lệnh cho trình biên dịch: "Cấm đoán mò! Mỗi lần dùng biến này
 *    phải đọc/ghi trực tiếp tại địa chỉ RAM/Thanh ghi thực tế."
 *
 * 2. CƠ CHẾ THANH GHI "WRITE-1-TO-CLEAR" (GHI 1 ĐỂ XÓA CỜ NGẮT) TRÊN STM32
 * ----------------------------------------------------------------------------
 * Trên các dòng chip thực tế như STM32 (ví dụ bộ DMA hoặc EXTI), nhà sản xuất tách riêng:
 *   - Thanh ghi đọc trạng thái ngắt: `ISR` (Interrupt Status Register - Chỉ đọc).
 *   - Thanh ghi xóa cờ ngắt:         `IFCR` (Interrupt Flag Clear Register - Ghi 1 để xóa).
 * Khi bạn ghi bit `1` vào một vị trí trong `IFCR` (ví dụ `dma->IFCR |= FLAG_BIT;`),
 * phần cứng của chip sẽ tự động xóa bit tương ứng bên trong thanh ghi `ISR` về `0`!
 * (Trong bài mô phỏng dưới đây, hàm `DMA_Hardware_SyncClear()` sẽ đóng vai phần cứng đó).
 *
 * 3. TỪ KHÓA `const` — TIẾT KIỆM RAM BẰNG CÁCH ĐẨY BẢNG DỮ LIỆU VÀO FLASH
 * ----------------------------------------------------------------------------
 * Vi điều khiển luôn rất nghèo RAM nhưng dư dả Flash (ví dụ STM32F103 chỉ có 20KB RAM).
 * Với các bảng hệ số cố định không bao giờ sửa lúc chạy (như Bảng tra cứu hiệu chuẩn
 * cảm biến - Lookup Table, hoặc mảng trọng số mạng nơ-ron TinyML), ta luôn khai báo:
 *     static const int16_t OFFSET_LUT[8] = { ... };
 * Từ khóa `const` vừa bảo vệ mảng không bị code ghi đè nhầm, vừa giúp trình biên dịch
 * cất toàn bộ mảng này vào bộ nhớ Flash (`.rodata`), tốn 0 byte RAM!
 *
 * 4. TỪ KHÓA `static` BÊN TRONG HÀM — GIỮ TRẠNG THÁI QUA CÁC LẦN GỌI HÀM
 * ----------------------------------------------------------------------------
 * Biến cục bộ bình thường bên trong hàm sẽ bị hủy ngay khi hàm chạy xong (`return`).
 * Nhưng nếu bạn thêm từ khóa `static` trước biến cục bộ:
 *     void MyFunc(void) {
 *         static uint32_t count = 0; // Chỉ khởi tạo = 0 đúng 1 lần lúc bật nguồn!
 *         count++;                   // Lần gọi thứ 1 -> 1; Lần gọi thứ 2 -> 2...
 *     }
 * Biến `static` nằm cố định trong RAM suốt vòng đời chương trình nhưng chỉ hàm đó
 * mới nhìn thấy được. Kỹ thuật này cực kỳ sạch sẽ (tránh xả rác biến toàn cục) khi làm:
 *   - Bộ phát hiện sườn xung (lưu trạng thái `prev_level` của lần đo trước).
 *   - Bộ đếm số sự kiện hoặc bộ lọc tín hiệu theo chu kỳ.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * BẢNG DATASHEET THANH GHI BỘ ĐIỀU KHIỂN DMA (`ISR` và `IFCR` - 32 bit)
 * +-------------+-------------+-------------+-------------+------------------+
 * |  Bit 31..4  |    Bit 3    |    Bit 2    |    Bit 1    |      Bit 0       |
 * +-------------+-------------+-------------+-------------+------------------+
 * |  Reserved   |    TEIF     |    HTIF     |    TCIF     |       GIF        |
 * |  (Luôn = 0) | (Lỗi truyền)| (Xong 1 nửa)| (Truyền xong)| (Cờ ngắt chung)  |
 * +-------------+-------------+-------------+-------------+------------------+
 * ============================================================================ */

#define DMA_GIF_BIT     (1UL << 0)  // Bit 0: Global Interrupt Flag
#define DMA_TCIF_BIT    (1UL << 1)  // Bit 1: Transfer Complete Interrupt Flag (Truyền xong)
#define DMA_HTIF_BIT    (1UL << 2)  // Bit 2: Half Transfer Interrupt Flag (Truyền được 1 nửa)
#define DMA_TEIF_BIT    (1UL << 3)  // Bit 3: Transfer Error Interrupt Flag (Lỗi đường truyền)

/* Cấu trúc mô phỏng cụm thanh ghi phần cứng DMA và dữ liệu chia sẻ với Ngắt */
typedef struct {
    volatile uint32_t ISR;              // Thanh ghi trạng thái ngắt (Phần cứng bật bit lên 1)
    volatile uint32_t IFCR;             // Thanh ghi xóa cờ ngắt (Phần mềm ghi 1 vào để xóa cờ ở ISR)
    volatile uint16_t latest_sample;    // Mẫu ADC 12-bit mới nhất do Ngắt thu được
    volatile bool     data_ready_flag;  // Cờ báo hiệu cho hàm main() biết đã có mẫu mới
    volatile uint32_t err_count;        // Bộ đếm số lần xảy ra lỗi đường truyền DMA
} DMA_Context_t;

/* Bảng tra cứu (Lookup Table - LUT) bù sai số cảm biến lưu trong bộ nhớ FLASH (const) */
static const int16_t SENSOR_CALIB_LUT[8] = {
    -15,  // Vùng 0: ADC từ 0    .. 511
    -8,   // Vùng 1: ADC từ 512  .. 1023
    -2,   // Vùng 2: ADC từ 1024 .. 1535
    0,    // Vùng 3: ADC từ 1536 .. 2047 (Vùng tuyến tính chuẩn)
    +3,   // Vùng 4: ADC từ 2048 .. 2559
    +9,   // Vùng 5: ADC từ 2560 .. 3071
    +18,  // Vùng 6: ADC từ 3072 .. 3583
    +30   // Vùng 7: ADC từ 3584 .. 4095
};

/* Hàm mô phỏng mạch phần cứng Write-1-to-Clear: Khi bạn ghi 1 vào IFCR, phần cứng tự xóa bit đó ở ISR */
static inline void DMA_Hardware_SyncClear(DMA_Context_t *dma) {
    if (dma != NULL) {
        dma->ISR &= ~(dma->IFCR); // Bit nào trong IFCR bằng 1 sẽ xóa bit tương ứng trong ISR về 0
        dma->IFCR = 0U;           // Sau khi xóa xong, thanh ghi IFCR tự trở về 0
    }
}


/* ============================================================================
 * PHẦN 2: 4 THỬ THÁCH THỰC HÀNH (45 PHÚT TỰ SUY NGHĨ VÀ CODE)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * THỬ THÁCH 1: XỬ LÝ TRONG HÀM PHỤC VỤ NGẮT DMA (`DMA_IRQHandler_Sim`)
 * Đặc tả kỹ thuật:
 *   - Nếu `dma == NULL`, thoát hàm ngay.
 *   - Bước 1 (Ưu tiên kiểm tra lỗi trước): Kiểm tra xem cờ lỗi `DMA_TEIF_BIT` (Bit 3)
 *     trong `dma->ISR` có đang bằng 1 hay không.
 *       + Nếu CÓ LỖI (`DMA_TEIF_BIT` == 1):
 *         1. Ghi bit `DMA_TEIF_BIT` vào thanh ghi `dma->IFCR` rồi gọi hàm `DMA_Hardware_SyncClear(dma);`
 *            để phần cứng xóa cờ lỗi.
 *         2. Tăng bộ đếm lỗi `dma->err_count` lên 1 đơn vị.
 *         3. Không nhận dữ liệu `new_adc_raw` và thoát hàm ngay (`return;`).
 *   - Bước 2 (Kiểm tra truyền hoàn tất): Kiểm tra xem cờ `DMA_TCIF_BIT` (Bit 1)
 *     trong `dma->ISR` có đang bằng 1 hay không.
 *       + Nếu ĐÃ TRUYỀN XONG (`DMA_TCIF_BIT` == 1):
 *         1. Ghi bit `DMA_TCIF_BIT` vào thanh ghi `dma->IFCR` rồi gọi `DMA_Hardware_SyncClear(dma);`
 *            để xóa cờ hoàn tất.
 *         2. Lưu giá trị `new_adc_raw` vào `dma->latest_sample`.
 *         3. Bật cờ `dma->data_ready_flag` lên `true` để báo cho vòng lặp `main()`.
 * ---------------------------------------------------------------------------- */
void DMA_IRQHandler_Sim(DMA_Context_t *dma, uint16_t new_adc_raw) {
    // TODO 1: Viết logic xử lý ngắt DMA tại đây
    if( dma == NULL ) return;
    if( (dma->ISR & ( 1U << 3)) != 0 ){
        uint8_t ghi_bit = ( dma->ISR >> 3) & 1U;
        dma->IFCR = (ghi_bit << 3);
        DMA_Hardware_SyncClear(dma);
        dma->err_count ++;
        return;

    } 
    if(( dma->ISR  & ( 1U << 1)) != 0 ){
        dma->IFCR = 1U << 1;
        DMA_Hardware_SyncClear(dma);
        dma->latest_sample = new_adc_raw;
        dma->data_ready_flag = true;
        
    }
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 2: HIỆU CHUẨN CẢM BIẾN BẰNG BẢNG TRA CỨU `const` TRONG FLASH
 * Đặc tả kỹ thuật:
 *   - Cảm biến xuất ra giá trị ADC 12-bit (`0 .. 4095`). Nếu đầu vào `adc_raw` lớn hơn
 *     `4095`, hãy giới hạn (kẹp biên) `adc_raw = 4095`.
 *   - Dải `0 .. 4095` được chia đều thành 8 vùng (mỗi vùng rộng `512` đơn vị, tức `2^9`).
 *     Hãy tìm chỉ số vùng `zone_idx` (`0 .. 7`) từ `adc_raw` bằng phép dịch bit (không dùng phép chia `/`).
 *   - Lấy giá trị gốc `adc_raw` (đã kẹp biên) cộng với độ lệch bù tương ứng trong bảng
 *     `SENSOR_CALIB_LUT[zone_idx]` và trả về kết quả kiểu `int16_t`.
 * ---------------------------------------------------------------------------- */
int16_t Sensor_CalibrateWithLUT(uint16_t adc_raw) {
    // TODO 2: Kẹp biên 4095, tìm zone_idx bằng dịch bit và cộng bù từ SENSOR_CALIB_LUT
    if( adc_raw > 4095) {
        adc_raw = 4095;
    }
    uint8_t zone_idx = (uint16_t)(adc_raw >> 9);

    return (uint16_t) adc_raw + SENSOR_CALIB_LUT[zone_idx];
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 3: BẮT SƯỜN LÊN (RISING EDGE) & ĐẾM XUNG BẰNG BIẾN CỤC BỘ `static`
 * Đặc tả kỹ thuật:
 *   - Không được khai báo thêm bất kỳ biến toàn cục nào bên ngoài hàm!
 *   - Sử dụng 2 biến cục bộ có từ khóa `static` bên trong hàm:
 *       + Một biến lưu trạng thái mức tín hiệu của lần gọi ngay trước đó (khởi tạo = `false`).
 *       + Một biến đếm tổng số sườn lên đã phát hiện được (khởi tạo = `0`).
 *   - Quy tắc hoạt động:
 *       + Nếu `reset == true`: Đặt lại bộ đếm về `0`, cập nhật trạng thái trước đó
 *         bằng `current_level`, và trả về `0`.
 *       + Nếu `reset == false`:
 *         Kiểm tra xem có xảy ra **Sườn lên (Rising Edge)** hay không (tức là lần gọi
 *         trước mức tín hiệu là `false` VÀ lần gọi hiện tại `current_level` là `true`).
 *         Nếu có sườn lên, tăng bộ đếm lên 1.
 *         Đừng quên cập nhật trạng thái trước đó bằng `current_level` trước khi trả về tổng số đếm!
 * ---------------------------------------------------------------------------- */
uint32_t Signal_CountRisingEdges(bool current_level, bool reset) {
    // TODO 3: Sử dụng biến cục bộ `static` để bắt sườn lên và đếm xung
    static bool status = false;
    static uint16_t count = 0; 
    if( reset == true){
        status = current_level;
        count = 0;
        return 0;
    } else {
        if(current_level == true && status == false){
            status = current_level;
            count ++;
        }
    }
    status = current_level;
    return count;
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 4: TIÊU THỤ DỮ LIỆU AN TOÀN TRONG VÒNG LẶP CHÍNH (`MainLoop_ConsumeData`)
 * Đặc tả kỹ thuật:
 *   - Nếu `dma == NULL` hoặc `out_calibrated == NULL`, trả về `false`.
 *   - Kiểm tra cờ báo hiệu `dma->data_ready_flag`:
 *       + Nếu cờ đang là `false` (chưa có mẫu mới từ Ngắt): không làm gì cả, trả về `false`.
 *       + Nếu cờ đang là `true`:
 *         1. Hạ cờ `dma->data_ready_flag` về `false` ngay lập tức để đánh dấu đã đọc.
 *         2. Đọc mẫu `dma->latest_sample`, đưa qua hàm `Sensor_CalibrateWithLUT()` ở Thử thách 2,
 *            rồi ghi kết quả vào vùng nhớ mà con trỏ `out_calibrated` trỏ tới.
 *         3. Trả về `true`.
 * ---------------------------------------------------------------------------- */
bool MainLoop_ConsumeData(DMA_Context_t *dma, int16_t *out_calibrated) {
    // TODO 4: Kiểm tra cờ data_ready_flag, hạ cờ, hiệu chuẩn mẫu và xuất qua con trỏ
    if( dma == NULL || out_calibrated == NULL ) return false;

    if( dma->data_ready_flag == false) return false;
    else {
        dma->data_ready_flag = false;
        *out_calibrated = Sensor_CalibrateWithLUT(dma->latest_sample);
        return true;
    }
    return false;
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG (KHÔNG SỬA ĐỔI HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;
    DMA_Context_t dma_ch1 = {
        .ISR             = 0U,
        .IFCR            = 0U,
        .latest_sample   = 0U,
        .data_ready_flag = false,
        .err_count       = 0U
    };

    printf("=== BAT DAU CHAM DIEM THU THACH NGAY 04 ===\n\n");

    /* --- Test Thử thách 1: Ngắt DMA & Write-1-to-Clear --- */
    // Kịch bản A: Giả lập phần cứng báo lỗi truyền (TEIF = 1) và cả TCIF = 1
    dma_ch1.ISR = (DMA_TEIF_BIT | DMA_TCIF_BIT);
    DMA_IRQHandler_Sim(&dma_ch1, 2000U);
    bool err_handled = ((dma_ch1.ISR & DMA_TEIF_BIT) == 0U) &&
                       (dma_ch1.err_count == 1U) &&
                       (dma_ch1.data_ready_flag == false);

    // Kịch bản B: Xóa sạch ISR, giả lập truyền thành công (TCIF = 1, HTIF = 1)
    dma_ch1.ISR = (DMA_TCIF_BIT | DMA_HTIF_BIT);
    DMA_IRQHandler_Sim(&dma_ch1, 2500U);
    bool tc_handled = ((dma_ch1.ISR & DMA_TCIF_BIT) == 0U) &&
                      ((dma_ch1.ISR & DMA_HTIF_BIT) != 0U) && // HTIF không bị xóa nhầm
                      (dma_ch1.latest_sample == 2500U) &&
                      (dma_ch1.data_ready_flag == true);

    if (err_handled && tc_handled) {
        printf("[PASS] Thu thach 1: Ham ngat DMA_IRQHandler_Sim xu ly uu tien loi va xoa co IFCR chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 1: err_handled=%d (ISR=0x%02X, err=%u), tc_handled=%d (sample=%u, rdy=%d)\n",
               err_handled, (unsigned int)dma_ch1.ISR, (unsigned int)dma_ch1.err_count,
               tc_handled, dma_ch1.latest_sample, dma_ch1.data_ready_flag);
    }

    /* --- Test Thử thách 2: Bảng tra cứu const LUT trong Flash --- */
    int16_t cal_z0  = Sensor_CalibrateWithLUT(100U);   // Zone 0 (100 >> 9 = 0)  -> 100 - 15 = 85
    int16_t cal_z3  = Sensor_CalibrateWithLUT(1800U);  // Zone 3 (1800 >> 9 = 3) -> 1800 + 0 = 1800
    int16_t cal_z7  = Sensor_CalibrateWithLUT(4000U);  // Zone 7 (4000 >> 9 = 7) -> 4000 + 30 = 4030
    int16_t cal_max = Sensor_CalibrateWithLUT(5000U);  // Vượt 4095 -> kẹp 4095 (Zone 7) -> 4095 + 30 = 4125

    if ((cal_z0 == 85) && (cal_z3 == 1800) && (cal_z7 == 4030) && (cal_max == 4125)) {
        printf("[PASS] Thu thach 2: Ham Sensor_CalibrateWithLUT tra bang Flash va dich bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 2: z0=%d(85), z3=%d(1800), z7=%d(4030), max=%d(4125)\n",
               cal_z0, cal_z3, cal_z7, cal_max);
    }

    /* --- Test Thử thách 3: Bắt sườn lên bằng biến static --- */
    // Chuỗi xung đưa vào: 0 -> 0 -> 1 (Sườn 1) -> 1 -> 0 -> 1 (Sườn 2) -> 0 -> 1 (Sườn 3)
    (void)Signal_CountRisingEdges(false, true);  // Reset về 0
    uint32_t c1 = Signal_CountRisingEdges(false, false); // 0 -> 0: count = 0
    uint32_t c2 = Signal_CountRisingEdges(true,  false); // 0 -> 1: count = 1 (Rising!)
    uint32_t c3 = Signal_CountRisingEdges(true,  false); // 1 -> 1: count = 1 (Giữ mức cao, không tăng)
    uint32_t c4 = Signal_CountRisingEdges(false, false); // 1 -> 0: count = 1 (Sườn xuống, không tăng)
    uint32_t c5 = Signal_CountRisingEdges(true,  false); // 0 -> 1: count = 2 (Rising!)
    uint32_t c6 = Signal_CountRisingEdges(true,  true);  // Reset khi đang ở mức true -> count = 0
    uint32_t c7 = Signal_CountRisingEdges(true,  false); // Vì lúc reset đã ở true, gọi tiếp true không tính là sườn lên -> count = 0

    if ((c1 == 0U) && (c2 == 1U) && (c3 == 1U) && (c4 == 1U) && (c5 == 2U) && (c6 == 0U) && (c7 == 0U)) {
        printf("[PASS] Thu thach 3: Ham Signal_CountRisingEdges giu trang thai static chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 3: c1=%u(0), c2=%u(1), c3=%u(1), c4=%u(1), c5=%u(2), c6=%u(0), c7=%u(0)\n",
               (unsigned int)c1, (unsigned int)c2, (unsigned int)c3,
               (unsigned int)c4, (unsigned int)c5, (unsigned int)c6, (unsigned int)c7);
    }

    /* --- Test Thử thách 4: Phối hợp Ngắt (Producer) và Main Loop (Consumer) --- */
    int16_t processed_val = 0;
    // Lúc này data_ready_flag đang là true và latest_sample = 2500 (Zone 4: 2500 >> 9 = 4 -> bù +3 -> 2503)
    bool first_read  = MainLoop_ConsumeData(&dma_ch1, &processed_val);
    // Đọc lần 2 liên tiếp khi chưa có ngắt mới -> phải trả về false vì cờ đã bị hạ về false
    bool second_read = MainLoop_ConsumeData(&dma_ch1, &processed_val);

    if ((first_read == true) && (processed_val == 2503) &&
        (dma_ch1.data_ready_flag == false) && (second_read == false)) {
        printf("[PASS] Thu thach 4: Ham MainLoop_ConsumeData tieu thu du lieu ngat an toan!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 4: first=%d, val=%d (ky vong 2503), rdy=%d (ky vong 0), second=%d (ky vong 0)\n",
               first_read, processed_val, dma_ch1.data_ready_flag, second_read);
    }

    printf("\n=== KET QUA TONG KET NGAY 04: %u / 4 THU THACH DAT [PASS] ===\n", pass_count);
    return 0;
}