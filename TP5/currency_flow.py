import dlt
import pandas as pd
from prefect import flow, task
from rest_api_pipeline import load_pipeline
import subprocess

@task
def run_dbt_models():
    """Runs dbt models to recalculate prices."""
    # Navigate to the dbt project directory
    dbt_project_path = "./currency_conversion"
    command = ["dbt", "run", "--project-dir", dbt_project_path]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    print(result.stdout)
    if result.stderr:
        print(result.stderr)

@flow(name="Currency Exchange Rate ETL")
def currency_exchange_rate_etl():
    """Main Prefect flow to fetch and load currency exchange rates and run dbt."""
    load_pipeline()
    run_dbt_models()

if __name__ == "__main__":

    # CRON expression for 'Every day at 1:00 AM' is "0 1 * * *"
    currency_exchange_rate_etl.serve(
        name="daily-data-deployment",
        cron="0 1 * * *", 
        description="Runs the DLT load and dbt transformations daily at 1 AM.",
        tags=["production", "daily-pipeline"]
    )
