# Dự Án Khai Phá Dữ Liệu: Khai Phá Tập Mục Phổ Biến (C++17)

Dự án cài đặt và thực nghiệm so sánh 3 thuật toán khai phá tập mục phổ biến kinh điển:
1. **Apriori** [AS94b] (Agrawal & Srikant, 1994)
2. **Eclat** [Zak00] (Mohammed J. Zaki, 2000)
3. **FP-growth** [HPY00] (Han, Pei, Yin, 2000)

---

## 1. Cấu Trúc Thư Mục

```text
DataMining_Project/
├── bin/                    # Chứa file thực thi DataMining_Project.exe
├── data/                   # Chứa các tập dữ liệu thực nghiệm
│   ├── chess.dat           # Dữ liệu rất đặc (3,196 tx, mật độ 49.33%)
│   ├── mushroom.dat        # Dữ liệu đặc vừa (8,416 tx, mật độ 19.33%)
│   └── retail.dat          # Dữ liệu rất thưa (88,162 tx, mật độ 0.06%)
├── include/                # Header files
│   ├── Apriori.h
│   ├── FPGrowth.h
│   ├── Eclat.h
│   └── DataUtils.h
├── src/                    # Source files
│   ├── Apriori.cpp
│   ├── FPGrowth.cpp
│   ├── Eclat.cpp
│   ├── DataUtils.cpp
│   └── main.cpp
├── CMakeLists.txt          # Cấu hình build CMake
├── REPORT.md               # Báo cáo phân tích chuyên sâu hiệu năng và các tình huống
└── README.md               # Hướng dẫn sử dụng
```

---

## 2. Hướng Dẫn Biên Dịch (Build)

### Cách 1: Sử dụng CMake (Khuyên dùng)
Yêu cầu: Máy đã cài đặt CMake (>= 3.20) và một trình biên dịch C++17 (MSVC hoặc MinGW g++).

```bash
# Di chuyển vào thư mục dự án
cd DataMining_Project

# Cấu hình dự án với CMake
cmake -B build -S .

# Biên dịch Release (tối ưu hóa tốc độ cao nhất /O2 hoặc -O3)
cmake --build build --config Release
```
Sau khi biên dịch, file thực thi sẽ được xuất trực tiếp vào thư mục `bin/DataMining_Project.exe`.

### Cách 2: Biên dịch trực tiếp bằng g++ (MinGW)
```bash
g++ -std=c++17 -O3 -Iinclude src/*.cpp -o bin/DataMining_Project.exe
```

---

## 3. Hướng Dẫn Chạy Chương Trình

### 3.1. Chạy Tự Động Toàn Bộ Benchmark Suite
Chương trình sẽ tự động nạp cả 3 tập dữ liệu (`chess.dat`, `mushroom.dat`, `retail.dat`), chạy từng thuật toán qua nhiều ngưỡng `min_sup`, đo đạc thời gian, bộ nhớ và xuất bảng so sánh chi tiết:

```bash
.\bin\DataMining_Project.exe -benchmark
```

### 3.2. Chạy Từng Thuật Toán Với Tham Số Tùy Chọn
Cú pháp lệnh:
```bash
.\bin\DataMining_Project.exe -data <đường_dẫn_dữ_liệu> -minsup <ngưỡng> [-alg <apriori|eclat|fpgrowth|all>] [-out <file_kết_quả>]
```

**Ví dụ:**

1. **Chạy so sánh cả 3 thuật toán trên `chess.dat` với min_sup = 90% (0.90):**
   ```bash
   .\bin\DataMining_Project.exe -data data/chess.dat -minsup 0.90 -alg all
   ```

2. **Chạy chỉ FP-Growth trên `mushroom.dat` với min_sup = 30% và xuất kết quả ra file `patterns_mushroom.txt`:**
   ```bash
   .\bin\DataMining_Project.exe -data data/mushroom.dat -minsup 0.30 -alg fpgrowth -out patterns_mushroom.txt
   ```

3. **Chạy Eclat trên `retail.dat` với min_sup tuyệt đối (Count = 1000):**
   ```bash
   .\bin\DataMining_Project.exe -data data/retail.dat -minsup 1000 -alg eclat
   ```

---

## 4. Báo Cáo Phân Tích

Vui lòng đọc file [`REPORT.md`](REPORT.md) để xem chi tiết:
- Phân tích chi tiết độ phức tạp thuật toán và cơ chế nén/duyệt.
- Bảng đo đạc thời gian (ms), bộ nhớ (MB), số mẫu trên từng tập dữ liệu.
- Phân tích 4 tình huống: Mật độ dữ liệu (Dense vs Sparse), Kích thước dữ liệu ($|D|$ và $|I|$), Ngưỡng độ hỗ trợ ($\text{min\_sup}$), và Phân bố độ dài giao dịch.
