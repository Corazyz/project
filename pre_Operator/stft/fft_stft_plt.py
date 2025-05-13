import matplotlib.pyplot as plt

# Read data from the file
x = []
y = []
with open("abs_output_2.txt", "r") as file:
    for line in file:
        index, value = line.split()
        x.append(int(index))
        y.append(float(value))
# with open("f_output.txt", "r") as file:
#     for line in file:
#         index, value = line.split()
#         x.append(int(index))
#         y.append(float(value))

# Debug: Print the data to verify
# print("Data read from file:")
# print("x:", x)
# print("y:", y)
print(x)
x = x[:len(x)//2]
y = y[:len(y)//2]
# Plot the data
plt.figure(figsize=(10, 6))
plt.plot(x, y, label="|F(k)|^2", marker='o')
plt.title("Frequency Spectrum (abs_F)")
plt.xlabel("Frequency Index (k)")
plt.ylabel("|F(k)|^2")
plt.grid(True)
plt.legend()

# Save the plot to a file
plt.savefig("abs_F_plot2.png")
plt.close()

print("Plot saved as abs_F_plot.png")
