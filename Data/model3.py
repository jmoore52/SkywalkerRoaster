import csv
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures
from sklearn.metrics import r2_score, mean_squared_error
import matplotlib.pyplot as plt

# Function to read CSV file and return X and y arrays
def read_csv(file_path):
    data = {'X1': [], 'X2': [], 'Y': []}
    with open(file_path, 'r') as file:
        csv_reader = csv.reader(file)
        for row in csv_reader:
            try:
                x1, x2, y = map(int, row)  # Assuming the CSV contains three integers per row
                data['X1'].append(x1/1000)
                data['X2'].append(x2/1000)
                data['Y'].append(y)
            except ValueError:
                print(f"Skipping invalid row: {row}")

    return data

# Example usage
csv_file_path = 'fullrealdataints.txt'  # Replace with the path to your CSV file
data = read_csv(csv_file_path)

# Convert data to numpy arrays
X1 = np.array(data['X1'])
X2 = np.array(data['X2'])
Y = np.array(data['Y'])


# Create polynomial features
degree = 4  # Adjust the degree as needed
poly = PolynomialFeatures(degree)
X_poly = poly.fit_transform(np.column_stack((X1, X2))) 

# Create a linear regression model
model = LinearRegression()

# Fit the model to the polynomial features
model.fit(X_poly, Y)

# Make predictions using the model
Y_pred = model.predict(X_poly)

# Print R-squared and Mean Squared Error
print(f"R-squared: {r2_score(Y, Y_pred)}")
print(f"Mean Squared Error: {mean_squared_error(Y, Y_pred)}")

residuals = Y - Y_pred


coefficients = model.coef_
intercept = model.intercept_

# Get feature names
feature_names = poly.get_feature_names_out(['X1', 'X2'])

# Display the linear regression equation
equation_parts = [f"{intercept}"]  # Initialize with intercept
for i, coef in enumerate(coefficients):  # Skip the intercept coefficient
    equation_parts.append(f" + {coef} * {feature_names[i]}")

equation = "".join(equation_parts)
print(f"Linear Regression Equation: Y = {equation}")

# Plot the original data points and the regression curve
plt.scatter(X1,X2, c=Y, cmap='viridis', marker='o', label='Original Data')
plt.scatter(X1,X2, c=Y_pred, cmap='viridis', marker='^', label='Regression Predictions')
plt.xlabel('X1')
plt.ylabel('X2')
plt.legend()
plt.show()


plt.scatter(Y_pred, residuals)
plt.axhline(y=0, color='red', linestyle='--', linewidth=2)  # Add a horizontal line at y=0
plt.xlabel('Predicted Values')
plt.ylabel('Residuals')
plt.title('Residuals vs. Predicted Values')
plt.show()


