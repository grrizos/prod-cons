import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("latency_results_60000.csv")  # or your combined file

plt.figure(figsize=(10, 6))
plt.errorbar(df["consumers"], df["avg_latency"], yerr=df["std_dev"],
             fmt='o-', capsize=4, capthick=2, elinewidth=1.5)

plt.title("Latency vs Number of Consumers")
plt.xlabel("Number of Consumers")
plt.ylabel("Average Latency (μs)")
plt.yscale("log")  # Optional: log scale to handle large values
plt.grid(True)
plt.xticks(df["consumers"])
plt.tight_layout()
plt.savefig("latency_plot_60000.png")
plt.show()
