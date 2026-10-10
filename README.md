# Data Mining Homework

Repository chứa các bài tập và đồ án môn **Khai phá dữ liệu (Data Mining)**.

---

## 📌 Dự Án Cốt Lõi: Khai Phá Tập Mục Phổ Biến (C++17)

Thư mục: [**`DataMining_Project/`**](DataMining_Project/)

Cài đặt và so sánh thực nghiệm hiệu năng của ba thuật toán kinh điển:
1. **Apriori** [AS94b] (Agrawal & Srikant, 1994)
2. **Eclat** [Zak00] (Mohammed J. Zaki, 2000) - Khai phá dữ liệu dọc
3. **FP-growth** [HPY00] (Han, Pei, Yin, 2000) - Cấu trúc cây tiền tố nén FP-Tree

---

### 📊 Bảng Kết Quả Thực Nghiệm Nhanh

Đo đạc trên 3 tập dữ liệu chuẩn FIMI / SPMF (Bản biên dịch Release `/O2` trên Windows):

| Tập dữ liệu | Phân loại | MinSup (%) | Số mẫu | Apriori (ms) | Eclat (ms) | FP-Growth (ms) | Thuật toán tối ưu |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **chess.dat** | Rất đặc (49.33%) | 80.0% | 8,282 | 672.86 ms | 54.98 ms | **15.26 ms** | **FP-Growth** (nhanh hơn 44x) |
| **mushroom.dat** | Khá đặc (19.33%) | 20.0% | 53,337 | 4,286.80 ms | 158.75 ms | **67.92 ms** | **FP-Growth** (nhanh hơn 63x) |
| **retail.dat** | Rất thưa (0.06%) | 1.0% | 159 | 82.16 ms | 132.75 ms | **53.02 ms** | **FP-Growth / Apriori** |

> 📖 **Xem báo cáo phân tích chi tiết đầy đủ:** [**`DataMining_Project/REPORT.md`**](DataMining_Project/REPORT.md)  
> 🛠️ **Xem hướng dẫn biên dịch và sử dụng:** [**`DataMining_Project/README.md`**](DataMining_Project/README.md)

---

### 🚀 Cách Chạy Nhanh

Mở terminal tại thư mục gốc và chạy:
```powershell
.\bin\DataMining_Project.exe -benchmark
```
Hoặc chạy trên từng tập dữ liệu:
```powershell
.\bin\DataMining_Project.exe -data data/chess.dat -minsup 0.90 -alg all
```
