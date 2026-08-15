OrangeSense 🍊

OrangeSense is a low-cost portable VIS/NIR spectroscopic device developed for the non-destructive estimation of orange sweetness (°Brix).

The system uses an AS7263 multispectral sensor to measure reflected light at six wavelengths (610, 680, 730, 760, 810, and 860 nm). Spectral data are converted into reflectance and absorbance values and processed using a Multiple Linear Regression (MLR) model to predict orange sweetness in real time.

The prediction model was developed from spectral measurements collected from 53 orange samples and deployed on an ESP32-S3 microcontroller, allowing standalone operation without the need for a computer.

![OrangeSense Circuit](https://github.com/wongnaphatkoedsin-svg/OrangeSense/blob/main/OrangeSense01.png)


Features

* Non-destructive sweetness measurement
* Real-time °Brix prediction
* Low-cost multispectral sensor (AS7263)
* Portable ESP32-S3 platform
* No internet connection required
* Suitable for Smart Agriculture applications

Hardware

* ESP32-S3
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

Brix = -32.421822 - 33.970504(R610) + 20.545394(R680) - 69.613837 (R730) + 131.275340 (R760) - 47.053119(A610) + 81.624047(A760) + 60.949098 (A810)

Calibration R² = 0.724 ; alibration RMSE= 1.15  °Brix; LOOCV R²= 0.614 ; LOOCV RMSE= 1.36 °Brix

Applications

* Fruit quality grading
* Harvest timing support
* Smart Agriculture
* Precision Agriculture
* Educational and research purposes

Authors

OrangeSense Project Tea
Phuket Wittayalai School, Phuket, Thailand

