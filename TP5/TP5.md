This project is an ETL (Extract, Transform, Load) pipeline designed to fetch currency exchange rates, process transaction logs, and transform them into a unified currency (USD) using `dlt` (data load tool) and `dbt` (data build tool), orchestrated by `Prefect`.

Here's a breakdown of the project components:

**1. `rest_api_pipeline.py`:**
   - **`get_currency_rates()`:** Extracts real-time exchange rates from the Frankfurter API (USD to EUR, GBP, JPY).
   - **`get_transaction_logs()`:** Reads historical transaction data from a CSV file hosted on Google Drive.
   - **`load_pipeline()`:**
     - Initializes a `dlt` pipeline with the destination set to `postgres` to load data into a PostgreSQL database.
     - Loads currency exchange rates into a `raw.currency_rates` table, replacing existing data on each run.
     - Loads transaction logs into a `raw.daily_transactions` table, appending new transactions.

**2. `currency_flow.py`:**
   - **`run_dbt_models()`:** Executes `dbt` models to transform the raw data. This task navigates to the `currency_conversion` dbt project and runs the models.
   - **`currency_exchange_rate_etl()`:** This is a Prefect flow that orchestrates the entire ETL process:
     - Calls `load_pipeline()` to extract and load data using `dlt`.
     - Calls `run_dbt_models()` to transform the loaded data using `dbt`.

**3. `currency_conversion/` (dbt project):**
   - **`dbt_project.yml`:** Configuration file for the dbt project, specifying project name, profile, and model paths.
   - **`models/example/schema.yml`:** Defines the sources for dbt models, linking to the `raw.daily_transactions` and `raw.currency_rates` tables created by `dlt`.
   - **`models/example/final_transaction_usd.sql`:** This dbt model performs the following transformations:
     - Selects the latest exchange rates from the `raw.currency_rates` table.
     - Selects raw transaction data from the `raw.daily_transactions` table.
     - Converts all transaction amounts to USD using the fetched exchange rates.

**4. `requirements.txt`:**
   - Lists all Python dependencies required for the project, including `dlt` (with `mysql` and `postgres` extras), `dbt-core`, `dbt-postgres`, `pandas`, `prefect`, and `mysql-connector-python`.

**Installation and Running Instructions:**

**Prerequisites:**

*   **Python 3.8+:** Ensure you have a compatible Python version installed.
*   **PostgreSQL Database:** You'll need a running PostgreSQL instance and a database where `dlt` can load the data.
*   **dbt CLI:** The dbt command-line interface needs to be installed.

**Steps:**

1.  **Clone the repository (if applicable):** If this project is in a Git repository, clone it to your local machine.
    ```bash
    git clone <repository_url>
    cd <project_directory>
    ```

2.  **Create a Python Virtual Environment (recommended):**
    ```bash
    python -m venv .venv
    source .venv/bin/activate
    ```

3.  **Install Python Dependencies:**
    ```bash
    pip install -r requirements.txt
    ```

4.  **Configure dbt Profile:**
    -   You need to configure your `profiles.yml` file for dbt to connect to your PostgreSQL database. This file is typically located in `~/.dbt/profiles.yml`.
    -   Add a profile named `currency_conversion` (as specified in `dbt_project.yml`) with your PostgreSQL connection details.

    Here's an example `profiles.yml` entry:

    ```yaml
    currency_conversion:
      target: dev
      outputs:
        dev:
          type: postgres
          host: localhost  # Your PostgreSQL host
          user: your_username
          password: your_password
          port: 5432       # Your PostgreSQL port
          dbname: your_database_name
          schema: public   # Or your desired schema
          threads: 1
    ```
    Replace `localhost`, `your_username`, `your_password`, `5432`, and `your_database_name` with your actual PostgreSQL credentials.

5.  **Run the Prefect Flow:**
    ```bash
    python currency_flow.py
    ```
    This will:
    -   Fetch currency rates and transaction logs.
    -   Load them into your PostgreSQL database using `dlt`.
    -   Run the dbt models to transform the data and create the `final_transaction_usd` table.

This project provides a robust framework for data ingestion and transformation, demonstrating the integration of `dlt`, `dbt`, and `Prefect` for building efficient data pipelines.