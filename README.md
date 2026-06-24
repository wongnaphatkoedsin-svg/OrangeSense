OrangeSense 🍊

OrangeSense is a low-cost portable VIS/NIR spectroscopic device developed for the non-destructive estimation of orange sweetness (°Brix).

The system uses an AS7263 multispectral sensor to measure reflected light at six wavelengths (610, 680, 730, 760, 810, and 860 nm). Spectral data are converted into reflectance and absorbance values and processed using a Multiple Linear Regression (MLR) model to predict orange sweetness in real time.

The prediction model was developed from spectral measurements collected from 57 orange samples and deployed on an ESP32-S3 microcontroller, allowing standalone operation without the need for a computer.

![OrangeSense Circuit](https://github.com/wongnaphatkoedsin-svg/OrangeSense/blob/main/OrangeSense01.png)


Features

* Non-destructive sweetness measurement
* Real-time °Brix prediction
* Low-cost multispectral sensor (AS7263)
* Portable ESP32-S3 platform
* No internet connection required
* Suitable for Smart Agriculture applications

Hardware

* PCBfun ESP32-S3
* AS7263 VIS/NIR Spectral Sensor
* 1.47” TFT LCD Display
* Push Button
* Lithium Battery

![OrangeSense Circuit](https://raw.githubusercontent.com/wongnaphatkoedsin-svg/OrangeSense/main/circuit.png)


Methodology

1. Measure white reference spectrum
2. Measure orange spectrum
3. Calculate Reflectance and Absorbance
4. Apply the MLR model
5. Display predicted °Brix on screen

Best MLR Model

Brix = -39.102377 - 38.803934(R610) + 20.472840(R680) + 74.262429(R760) - 46.117035(A610) + 102.133231(A730) + 47.008893(A810)

Calibration R² = 0.6337; alibration RMSE= 1.4397 °Brix; LOOCV R²= 0.5435; LOOCV RMSE= 1.6072 °Brix

Applications

* Fruit quality grading
* Harvest timing support
* Smart Agriculture
* Precision Agriculture
* Educational and research purposes

Authors

OrangeSense Project Team (Wongnathat Koedsin)
Phuket Wittayalai School, Phuket, Thailand

