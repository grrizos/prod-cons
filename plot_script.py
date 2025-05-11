import pandas as pd
import numpy as np

def load_data(filename):
    """Load CSV, handling lines with extra columns"""
    data = []
    with open(filename, 'r') as f:
        for line in f:
            parts = line.strip().split(',')
            if len(parts) >= 2:  # Take first 2 columns only
                try:
                    latency = int(parts[0])
                    consumers = int(parts[1])
                    data.append([latency, consumers])
                except (ValueError, IndexError):
                    continue
    return pd.DataFrame(data, columns=["latency", "consumers"])

def process_data(consumer_count, df):
    df_filtered = df[df["consumers"] == consumer_count]
    if len(df_filtered) == 0:
        return None, None
    return df_filtered["latency"].mean(), df_filtered["latency"].std()

# Load data (with error-tolerant parsing)
try:
    df = load_data("latency_data.csv")
except Exception as e:
    print(f"Failed to load data: {e}")
    exit(1)

# Process and save results
results = []
for q in [1, 2, 4, 8, 16]:
    avg, std = process_data(q, df)
    if avg is not None:
        results.append({
            "consumers": q,
            "avg_latency": avg,
            "std_dev": 0 if pd.isna(std) else std
        })

if results:
    pd.DataFrame(results).to_csv("latency_results_60000.csv", index=False)
    print(f"Processed {len(df)} records. Results saved.")
else:
    print("No valid data found!")