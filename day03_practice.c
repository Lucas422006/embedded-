/**
 * ============================================================================
 * THỬ THÁCH EMBEDDED C - NGÀY 03 (Thời lượng: 60 phút - Chế độ Tự Suy Nghĩ)
 * Chủ đề: Tự thiết kế Bitmask từ Datasheet, Trích xuất Cụm Bit & Giải mã Gói tin
 * Tài liệu tra cứu chuẩn:
 *   - Ngôn ngữ C (Operators / Bitwise / Fixed-width): https://cppreference.net/c/language.html
 *   - Quy trình lưu phiên bản với Git:                https://git-scm.com/cheat-sheet
 * ============================================================================
 * QUY TẮC THỬ THÁCH TỐI NAY:
 * - File này KHÔNG cung cấp sẵn công thức dịch bit hay các hằng số #define.
 * - Bạn hãy đóng vai kỹ sư Firmware đọc bảng Datasheet bên dưới, tự định nghĩa
 *   các #define mặt nạ bit và hoàn thiện 4 hàm theo đúng đặc tả đầu vào/đầu ra.
 * - Biên dịch và chạy chương trình để kiểm tra với bộ Test tự động ở hàm main().
 *   Mục tiêu: Đạt tuyệt đối 4 / 4 [PASS] rồi đẩy lên GitHub.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * BẢNG DATASHEET 1: THANH GHI ĐIỀU KHIỂN `ctrl_reg` (16-bit: Bit 15 .. Bit 0)
 * +-------------+-------------+---------------+---------------+--------------+
 * |  Bit 15..8  |    Bit 7    |    Bit 6      |   Bit 5..4    |   Bit 3..0   |
 * +-------------+-------------+---------------+---------------+--------------+
 * |  Reserved   |  SOFT_RESET |  FIFO_ENABLE  |   FS_RANGE    |   ODR_RATE   |
 * | (Luôn = 0)  | (1: Reset)  | (1:En, 0:Dis) | (0..3: Thang) | (0..15: Tần) |
 * +-------------+-------------+---------------+---------------+--------------+
 *
 * BẢNG DATASHEET 2: THANH GHI TRẠNG THÁI `status_reg` (8-bit: Bit 7 .. Bit 0)
 * +-------------+-------------+----------------------------------------------+
 * |    Bit 7    |   Bit 6..1  |                    Bit 0                     |
 * +-------------+-------------+----------------------------------------------+
 * |   CRC_ERR   |  Reserved   |                  DATA_RDY                    |
 * | (1: Lỗi gói)| (Luôn = 0)  |              (1: Có dữ liệu mới)             |
 * +-------------+-------------+----------------------------------------------+
 * ============================================================================ */

/* TODO 0: Tự viết các #define vị trí bit (_POS) và mặt nạ bit (_MASK / _BIT) tại đây */
#define CTRL_ODR_RATE_POS ( 0U )
#define CTRL_ODR_RATE_MASK ( 15U << CTRL_ODR_RATE_POS  ) // bit 0 - 3 
#define CTRL_FS_RANGE_POS ( 4U)
#define CTRL_FS_RANGE_MASK ( 3U << CTRL_FS_RANGE_POS )
#define CTRL_FIFO_ENABLE ( 1U << 6) 
#define CTRL_SOFT_RESET ( 1U << 7)
//#define CTRL_Reserved ( 15U << 8)
#define STATUS_REG_DATA_RDY ( 1U << 0 )
#define STATUS_REG_CRC_ERR ( 1U << 7 )


typedef struct {
    uint16_t ctrl_reg;      // Thanh ghi điều khiển 16-bit
    uint8_t  status_reg;    // Thanh ghi trạng thái 8-bit
    int16_t  accel_x;       // Gia tốc trục X sau giải mã
    int16_t  accel_y;       // Gia tốc trục Y sau giải mã
} IMU_Handle_t;


/* ============================================================================
 * THỬ THÁCH 1: CẤU HÌNH ĐỒNG THỜI NHIỀU TRƯỜNG BIT TRONG `ctrl_reg`
 * Đặc tả kỹ thuật:
 *   - Nếu `imu == NULL`, hoặc `odr_code` lớn hơn 15, hoặc `fs_code` lớn hơn 3:
 *     không làm gì cả và thoát hàm ngay.
 *   - Cập nhật đồng thời 3 thông số vào `imu->ctrl_reg` mà KHÔNG làm thay đổi
 *     các bit khác (như Bit 7 SOFT_RESET nếu đang có):
 *       1. Ghi `odr_code` (4 bit) vào trường `ODR_RATE` (Bit 3..0).
 *       2. Ghi `fs_code` (2 bit) vào trường `FS_RANGE` (Bit 5..4).
 *       3. Nếu `fifo_en == true` thì bật `FIFO_ENABLE` (Bit 6) lên 1,
 *          nếu `fifo_en == false` thì tắt `FIFO_ENABLE` (Bit 6) về 0.
 * ============================================================================ */
void IMU_Configure(IMU_Handle_t *imu, uint8_t odr_code, uint8_t fs_code, bool fifo_en) {
    // TODO 1: Tự suy nghĩ và triển khai tại đây
    if( imu == NULL || odr_code > 15 || fs_code > 3 ) return;
    uint16_t write = imu->ctrl_reg;
    uint16_t clear_mask = CTRL_FIFO_ENABLE|CTRL_FS_RANGE_MASK|CTRL_ODR_RATE_MASK;

    write &= ~ clear_mask;
    write |= (((uint16_t)odr_code << CTRL_ODR_RATE_POS ) | ((uint16_t)fs_code << CTRL_FS_RANGE_POS ));
    if( fifo_en ){
        write |= (1U << 6);
    }
    imu->ctrl_reg = write;
}


/* ============================================================================
 * THỬ THÁCH 2: TRÍCH XUẤT (EXTRACT) GIÁ TRỊ CỤM BIT Ở GIỮA THANH GHI
 * Đặc tả kỹ thuật:
 *   - Nếu `imu == NULL`, trả về `0xFF`.
 *   - Đọc riêng giá trị của trường `FS_RANGE` nằm ở Bit 5..4 của `imu->ctrl_reg`
 *     và trả về con số nguyên từ 0 đến 3 (tương ứng mã thang đo hiện tại).
 * ============================================================================ */
uint8_t IMU_GetFullScaleRange(const IMU_Handle_t *imu) {
    // TODO 2: Tự suy nghĩ cách lấy 2 bit ở vị trí Bit 5..4 và đưa về giá trị 0..3
    if( imu == NULL ) return 0xFF;
    uint8_t two_bit = ( imu->ctrl_reg >> 4 ) & 3U;
    
    return two_bit;
}


/* ============================================================================
 * THỬ THÁCH 3: KIỂM TRA CHECKSUM XOR & GIẢI MÃ GÓI TIN HỖN HỢP (BIG / LITTLE ENDIAN)
 * Đặc tả khung truyền 6 byte `raw_buf[0..5]` từ cảm biến:
 *   - Byte [0]    : Header cố định phải bằng `0xAA`.
 *   - Byte [1..2] : Giá trị `accel_x` (16-bit có dấu) truyền theo chuẩn BIG-ENDIAN
 *                   (Byte [1] là 8 bit cao MSB, Byte [2] là 8 bit thấp LSB).
 *   - Byte [3..4] : Giá trị `accel_y` (16-bit có dấu) truyền theo chuẩn LITTLE-ENDIAN
 *                   (Byte [3] là 8 bit thấp LSB, Byte [4] là 8 bit cao MSB).
 *   - Byte [5]    : Byte kiểm tra lỗi `Checksum`, được tính bằng phép XOR (`^`)
 *                   liên tiếp của 5 byte đầu: `raw_buf[0] ^ raw_buf[1] ^ raw_buf[2] ^ raw_buf[3] ^ raw_buf[4]`.
 *
 * Yêu cầu xử lý:
 *   - Nếu `imu == NULL` hoặc `raw_buf == NULL`: trả về `false`.
 *   - Kiểm tra tính hợp lệ của gói tin:
 *       + Nếu `raw_buf[0] != 0xAA` HOẶC kết quả tính XOR 5 byte đầu khác `raw_buf[5]`:
 *         Bật bit `CRC_ERR` (Bit 7) lên 1, xóa bit `DATA_RDY` (Bit 0) về 0 trong `imu->status_reg`,
 *         không cập nhật `accel_x`, `accel_y` và trả về `false`.
 *       + Nếu gói tin hợp lệ:
 *         Ghép byte để lưu vào `imu->accel_x` và `imu->accel_y` đúng chuẩn Endian tương ứng,
 *         xóa bit `CRC_ERR` (Bit 7) về 0, bật bit `DATA_RDY` (Bit 0) lên 1 trong `imu->status_reg`,
 *         và trả về `true`.
 * ============================================================================ */
bool IMU_ParsePacket(IMU_Handle_t *imu, const uint8_t raw_buf[6]) {
    // TODO 3: Tự suy nghĩ và triển khai tại đây
    if( imu == NULL || raw_buf == NULL ) return false;

    uint8_t ket_qua = 0;
    for( int i = 0; i < 5; i++){
        ket_qua ^= raw_buf[i];
    }
    if( raw_buf[0] != 0xAA || ket_qua != raw_buf[5] ){
        imu->status_reg |= ( 1U << 7) ;
        imu->status_reg &= ~ (1U << 0);
        return false;
    } else {
        imu->accel_x = (int16_t)(((int16_t)raw_buf[1] << 8  ) | (int16_t)raw_buf[2]);
        imu->accel_y = (int16_t)(((int16_t)raw_buf[4] << 8 ) | (int16_t)raw_buf[3]);
        imu->status_reg &= ~ (1U << 7);
        imu->status_reg |= (1U << 0);
        return true;
    }
    return false;
}


/* ============================================================================
 * THỬ THÁCH 4: LỌC VÙNG CHẾT (DEADZONE) & KẸP BIÊN TÍN HIỆU (CLAMPING / SATURATION)
 * Đặc tả kỹ thuật (Tiền xử lý dữ liệu trước khi đưa vào mô hình TinyML):
 *   - Đầu vào: Giá trị cảm biến `val`, ngưỡng vùng chết `deadzone` (số dương),
 *     và biên độ tối đa `max_abs` (số dương, lớn hơn `deadzone`).
 *   - Quy tắc:
 *       1. Nếu giá trị tuyệt đối của `val` nằm trong khoảng `(-deadzone, +deadzone)`
 *          (tức là lớn hơn `-deadzone` VÀ nhỏ hơn `+deadzone`): coi là nhiễu nền, trả về `0`.
 *       2. Nếu `val` lớn hơn `max_abs`: giới hạn (kẹp biên) trả về `max_abs`.
 *       3. Nếu `val` nhỏ hơn `-max_abs`: giới hạn (kẹp biên) trả về `-max_abs`.
 *       4. Các trường hợp còn lại: giữ nguyên `val`.
 * ============================================================================ */
int16_t IMU_ApplyDeadzoneAndClamp(int16_t val, int16_t deadzone, int16_t max_abs) {
    // TODO 4: Tự suy nghĩ và triển khai tại đây
    if( val < deadzone && val > ( (~deadzone) - 1 ) ) return 0;
    if( val > max_abs) return max_abs;
    if( val < ( ( ~max_abs) - 1)) return -max_abs;
    return val;
}


/* ============================================================================
 * HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG (KHÔNG SỬA ĐỔI HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;
    IMU_Handle_t dev = {
        .ctrl_reg   = (1U << 7), // Giả sử Bit 7 đang bằng 1 (0x0080) để kiểm tra bảo toàn bit
        .status_reg = 0x00,
        .accel_x    = 0,
        .accel_y    = 0
    };

    printf("=== BAT DAU CHAM DIEM THU THACH NGAY 03 ===\n\n");

    /* --- Test Thử thách 1 --- */
    // Cấu hình: ODR = 0x0A (1010b), FS = 0x02 (10b ở Bit 5..4 -> 0x20), FIFO = true (Bit 6 -> 0x40)
    // Kỳ vọng ctrl_reg = 0x80 | 0x40 | 0x20 | 0x0A = 0x00EA
    IMU_Configure(&dev, 0x0A, 0x02, true);
    uint16_t cfg1 = dev.ctrl_reg;
    // Đổi cấu hình: ODR = 0x05, FS = 0x01 (0x10), FIFO = false (Bit 6 = 0)
    // Kỳ vọng ctrl_reg = 0x80 | 0x00 | 0x10 | 0x05 = 0x0095
    IMU_Configure(&dev, 0x05, 0x01, false);
    uint16_t cfg2 = dev.ctrl_reg;
    // Thử truyền tham số sai (odr_code = 20 > 15) -> ctrl_reg phải giữ nguyên 0x0095
    IMU_Configure(&dev, 20, 0x03, true);
    uint16_t cfg_invalid = dev.ctrl_reg;

    if ((cfg1 == 0x00EAU) && (cfg2 == 0x0095U) && (cfg_invalid == 0x0095U)) {
        printf("[PASS] Thu thach 1: Ham IMU_Configure ghi da truong bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 1: cfg1=0x%04X (ky vong 0x00EA), cfg2=0x%04X (ky vong 0x0095), invalid=0x%04X\n",
               cfg1, cfg2, cfg_invalid);
    }

    /* --- Test Thử thách 2 --- */
    dev.ctrl_reg = 0x00B5U; // 0b1011_0101 -> Bit 5..4 là 11b (3)
    uint8_t fs_val1 = IMU_GetFullScaleRange(&dev);
    dev.ctrl_reg = 0x006FU; // 0b0110_1111 -> Bit 5..4 là 10b (2)
    uint8_t fs_val2 = IMU_GetFullScaleRange(&dev);

    if ((fs_val1 == 3U) && (fs_val2 == 2U) && (IMU_GetFullScaleRange(NULL) == 0xFFU)) {
        printf("[PASS] Thu thach 2: Ham IMU_GetFullScaleRange trich xuat cum bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 2: fs_val1=%u (ky vong 3), fs_val2=%u (ky vong 2)\n", fs_val1, fs_val2);
    }

    /* --- Test Thử thách 3 --- */
    // Gói tin hợp lệ:
    // Header = 0xAA
    // Accel_X (Big-Endian)    = 0xFF, 0x38 -> -200
    // Accel_Y (Little-Endian) = 0xDC, 0x05 -> 0x05DC = 1500
    // Checksum = 0xAA ^ 0xFF ^ 0x38 ^ 0xDC ^ 0x05 = 0xB4
    const uint8_t valid_pkt[6] = {0xAA, 0xFF, 0x38, 0xDC, 0x05, 0xB4};
    bool pkt_ok = IMU_ParsePacket(&dev, valid_pkt);
    uint8_t st_ok = dev.status_reg;
    int16_t ax_ok = dev.accel_x;
    int16_t ay_ok = dev.accel_y;

    // Gói tin sai Checksum (Byte cuối bị nhiễu thành 0x00)
    const uint8_t bad_pkt[6] = {0xAA, 0x01, 0x02, 0x03, 0x04, 0x00};
    bool pkt_bad = IMU_ParsePacket(&dev, bad_pkt);
    uint8_t st_bad = dev.status_reg;

    if ((pkt_ok == true) && (st_ok == 0x01U) && (ax_ok == -200) && (ay_ok == 1500) &&
        (pkt_bad == false) && (st_bad == 0x80U) && (dev.accel_x == -200)) {
        printf("[PASS] Thu thach 3: Ham IMU_ParsePacket kiem tra XOR va giai ma Big/Little Endian chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 3: pkt_ok=%d, st_ok=0x%02X, ax=%d (ky vong -200), ay=%d (ky vong 1500) | pkt_bad=%d, st_bad=0x%02X (ky vong 0x80)\n",
               pkt_ok, st_ok, ax_ok, ay_ok, pkt_bad, st_bad);
    }

    /* --- Test Thử thách 4 --- */
    int16_t t1 = IMU_ApplyDeadzoneAndClamp(12, 15, 1000);     // Trong vùng chết -> 0
    int16_t t2 = IMU_ApplyDeadzoneAndClamp(-14, 15, 1000);    // Trong vùng chết âm -> 0
    int16_t t3 = IMU_ApplyDeadzoneAndClamp(15, 15, 1000);     // Chạm biên vùng chết -> giữ nguyên 15
    int16_t t4 = IMU_ApplyDeadzoneAndClamp(1450, 15, 1000);   // Vượt ngưỡng dương -> 1000
    int16_t t5 = IMU_ApplyDeadzoneAndClamp(-2100, 15, 1000);  // Vượt ngưỡng âm -> -1000
    int16_t t6 = IMU_ApplyDeadzoneAndClamp(-420, 15, 1000);   // Bình thường -> -420

    if ((t1 == 0) && (t2 == 0) && (t3 == 15) && (t4 == 1000) && (t5 == -1000) && (t6 == -420)) {
        printf("[PASS] Thu thach 4: Ham IMU_ApplyDeadzoneAndClamp xu ly tin hieu chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 4: t1=%d(0), t2=%d(0), t3=%d(15), t4=%d(1000), t5=%d(-1000), t6=%d(-420)\n",
               t1, t2, t3, t4, t5, t6);
    }

    printf("\n=== KET QUA TONG KET: %u / 4 THU THACH DAT [PASS] ===\n", pass_count);
    return 0;
}