import requests
import pandas as pd
import datetime as dt
from typing import Iterator, Any, Dict

# --- 1) API Data Source (Exchange Rates) ---
def get_currency_rates() -> Iterator[Dict[str, Any]]:
    """Extracts latest USD->EUR/GBP/JPY rates and yields a normalized record with inverses."""
    API_URL = "https://api.frankfurter.dev/v1/latest"
    params = {"base": "USD", "symbols": "EUR,GBP,JPY"}

    resp = requests.get(API_URL, params=params, timeout=15)
    resp.raise_for_status()
    data = resp.json()  # {"amount":1.0,"base":"USD","date":"YYYY-MM-DD","rates":{...}}

    rates = data.get("rates", {})
    as_of = dt.datetime.fromisoformat(f"{data.get('date')}T00:00:00")

    record = {
        "as_of": as_of,
        "base": data.get("base", "USD"),
        "EUR": rates.get("EUR"),
        "GBP": rates.get("GBP"),
        "JPY": rates.get("JPY"),
        # inverses لتسهيل التحويل بالضرب بدل القسمة
        "usd_per_eur": (1 / rates["EUR"]) if "EUR" in rates else None,
        "usd_per_gbp": (1 / rates["GBP"]) if "GBP" in rates else None,
        "usd_per_jpy": (1 / rates["JPY"]) if "JPY" in rates else None,
    }
    # dlt.run يقبل iterator؛ ننتج صفًا واحدًا منسَّقًا
    yield record


# --- 2) CSV Data Source (Transaction Logs) ---
def get_transaction_logs() -> pd.DataFrame:
    """Reads transactions from the Google Drive CSV file."""
    return pd.read_csv(
        "https://drive.google.com/uc?export=download&id=1oY9CIYmtY0nL78bVp3lxvCWVDKMuhFv7"
    )


# --- 3) Run the dlt Pipeline ---
def load_pipeline():
    # ننقل الاستيراد هنا لتجنّب الخطأ عند استيراد الملف فقط
    import dlt

    # Initialize a dlt pipeline with the destination set to 'postgres'
    pipeline = dlt.pipeline(
        pipeline_name="currency_conversion",
        destination="postgres",
        dataset_name="raw",  # Load data into the 'raw' schema
    )

    # 1) Load API data → raw.currency_rates (REPLACE)
    rate_info = get_currency_rates()
    load_info_rates = pipeline.run(
        rate_info,
        table_name="currency_rates",
        write_disposition="replace",
    )

    # 2) Load CSV data → raw.daily_transactions (APPEND)
    transactions = get_transaction_logs()
    load_info_transactions = pipeline.run(
        transactions,
        table_name="daily_transactions",
        write_disposition="append",
    )

    print("--- Load Success! ---")
    print(load_info_rates)
    print(load_info_transactions)


if __name__ == "__main__":
    load_pipeline()
