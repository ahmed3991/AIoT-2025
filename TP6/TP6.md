# TP6: Northwind Data Mart with dlt and dbt

## Objective

This practical work aims to guide you through the process of building a data mart for Northwind's `Order Details` fact table, integrating it with related dimensions such as `Product`, `Customer`, `Order Date`, `Shipped Date`, and `Employees`. You will use `dlt` to extract data from a SQLite database and load it into a Supabase PostgreSQL database, and then use `dbt` to transform this raw data into a structured data warehouse.

## Part 1: Data Ingestion with dlt (from SQLite to Supabase)

In this section, you will set up a Python environment, install necessary libraries, configure `dlt` to connect to your Supabase PostgreSQL database, and then use a provided script to load initial data. You will then extend this script to load all necessary Northwind tables.

### 1.1 Environment Setup

1.  **Create and Activate a Virtual Environment:**
    It's good practice to work within a virtual environment to manage dependencies.
    ```bash
    python3 -m venv .venv
    source .venv/bin/activate
    ```

2.  **Install Required Python Packages:**
    Install `dlt`, `SQLAlchemy` (for SQLite connection), `pandas` (if needed for data manipulation, though `dlt` can handle rows directly).
    ```bash
    pip install -r requirements.txt
    ```

### 1.2 Configure dlt for Supabase (PostgreSQL)

`dlt` needs to know how to know how to connect to your Supabase PostgreSQL database. This is typically done via a `secrets.toml` file.

1.  **Locate/Create `secrets.toml`:**
    A `.dlt` directory with a `secrets.toml` file should already exist in your `TP6/` directory. If not, create it:
    ```
    TP6/
    └── .dlt/
        └── secrets.toml
    ```

2.  **Update `secrets.toml` with your Supabase Credentials:**
    Open `TP6/.dlt/secrets.toml` and ensure it contains your Supabase PostgreSQL connection details. Replace the placeholder values with your actual credentials. An example is provided below (you should use the one already in your workspace):

    ```toml
    [destination.postgres.credentials]
    database = "postgres" # Your Supabase database name (usually 'postgres')
    password = "YOUR_SUPABASE_PASSWORD" # Your actual Supabase database password
    username = "YOUR_SUPABASE_USERNAME" # Your actual Supabase database username
    host = "YOUR_SUPABASE_HOST" # Your Supabase host, e.g., 'aws-1-eu-west-1.pooler.supabase.com'
    port = 5432
    connect_timeout = 30
    ```
    **Important:** Keep your `secrets.toml` file secure and never commit it to public repositories with actual credentials.

### 1.3 Initial Data Load (Customers and Orders)

A Python script `sqlite_pipeline.py` is provided to load `Customers` and `Orders` data from a local SQLite database (`northwind.db`) into your Supabase `staging` schema.

1.  **Review `sqlite_pipeline.py`:**
    Examine the `TP6/sqlite_pipeline.py` file. Understand how it connects to the SQLite database, extracts data, and uses `dlt.pipeline().run()` to load it into PostgreSQL.

2.  **Run the Initial Pipeline:**
    Execute the script from your terminal:
    ```bash
    python TP6/sqlite_pipeline.py
    ```
    This will create a `staging` schema in your Supabase database and load the `Customers` and `Orders` tables.

### 1.4 Extend Data Load to All Northwind Tables

The `sqlite_pipeline.py` script currently loads only `Customers` and `Orders`. Your task is to extend this script to load all other relevant Northwind tables into the `staging` schema of your Supabase database.

**Tables to Load:**
*   `Employees`
*   `Products`
*   `Categories`
*   `Order Details`
*   `Orders`

**Instructions:**
1.  Open `TP6/sqlite_pipeline.py`.
2.  Add similar `with engine.connect() as conn:` blocks for each of the remaining tables.
3.  For each table, construct the appropriate `SELECT * FROM TableName` query.
4.  Ensure each `pipeline.run()` call specifies the correct `table_name`.
5.  After modifying, run the script again to load all data:
    ```bash
    python TP6/sqlite_pipeline.py
    ```
    Verify in your Supabase database that all tables are now present in the `staging` schema.

## Part 2: Data Transformation with dbt (Staging and Marts Layers)

In this section, you will use `dbt` to transform the raw data loaded into your Supabase `staging` schema into a structured data warehouse, consisting of `staging` models and `marts` (dimension and fact) models.

### 2.1 dbt Project Setup and Configuration

1.  **Navigate to the dbt Project:**
    Your dbt project for Northwind is located at `TP6/northwind_T/`. Navigate into this directory:
    ```bash
    cd TP6/northwind_T/
    ```

2.  **Configure `profiles.yml`:**
    `dbt` needs to know how to connect to your Supabase PostgreSQL database. This is configured in `~/.dbt/profiles.yml`. If you don't have one, `dbt init` will guide you.
    Ensure your `profiles.yml` has an entry for `northwind_T` (or whatever your dbt project name is) that points to your Supabase database. An example configuration:

    ```yaml
    northwind_T: # This should match the name in dbt_project.yml
      target: dev
      outputs:
        dev:
          type: postgres
          host: YOUR_SUPABASE_HOST
          user: YOUR_SUPABASE_USERNAME
          password: YOUR_SUPABASE_PASSWORD
          port: 5432
          dbname: postgres
          schema: northwind_warehouse # Or your preferred dbt output schema
          threads: 4
          keepalives_idle: 0 # Set to 0 to disable keepalives
    ```
    **Important:** Replace `YOUR_SUPABASE_HOST`, `YOUR_SUPABASE_USERNAME`, and `YOUR_SUPABASE_PASSWORD` with your actual Supabase credentials.

3.  **Test dbt Connection:**
    Run `dbt debug` to verify your connection to Supabase:
    ```bash
    dbt debug
    ```
    You should see `Connection test: OK`.

### 2.2 Staging Layer (Cleaning and Standardizing)

The `staging` layer focuses on cleaning, renaming, and standardizing raw data from the `dlt` `staging` schema.

1.  **Review Existing Staging Models:**
    Examine `TP6/northwind_T/models/staging/schema.yml`. This file defines the expected structure and tests for `stg_customers` and `stg_orders`.
    You will also find `TP6/northwind_T/models/staging/stg_customers.sql` and `TP6/northwind_T/models/staging/stg_orders.sql` (these files are not in the provided workspace structure, but are implied by the schema.yml). You will need to create them.

2.  **Create `stg_customers.sql` and `stg_orders.sql`:**
    Create the following files in `TP6/northwind_T/models/staging/`:

    **`stg_customers.sql`:**
    ```sql
    with source as (
        select * from {{ source('staging_dlt', 'Customers') }}
    ),
    renamed as (
        select
            {{ dbt_utils.surrogate_key(['CustomerID']) }} as customer_pk,
            "CustomerID" as customer_id,
            "CompanyName" as company_name,
            "ContactName" as contact_name,
            "ContactTitle" as contact_title,
            "Address" as address,
            "City" as city,
            "Region" as region,
            "PostalCode" as postal_code,
            "Country" as country,
            "Phone" as phone,
            "Fax" as fax
        from source
    )
    select * from renamed
    ```

    **`stg_orders.sql`:**
    ```sql
    with source as (
        select * from {{ source('staging_dlt', 'Orders') }}
    ),
    renamed as (
        select
            {{ dbt_utils.surrogate_key(['OrderID']) }} as order_pk,
            "OrderID" as order_id,
            "CustomerID" as customer_id,
            "EmployeeID" as employee_id,
            "OrderDate" as order_date,
            "RequiredDate" as required_date,
            "ShippedDate" as shipped_date,
            "ShipVia" as ship_via,
            "Freight" as freight,
            "ShipName" as ship_name,
            "ShipAddress" as ship_address,
            "ShipCity" as ship_city,
            "ShipRegion" as ship_region,
            "ShipPostalCode" as ship_postal_code,
            "ShipCountry" as ship_country
        from source
    )
    select * from renamed
    ```

3.  **Extend Staging Models for Other Tables:**
    Create `stg_employees.sql`, `stg_products.sql`, `stg_suppliers.sql`, `stg_categories.sql`, and `stg_order_details.sql` in the `TP6/northwind_T/models/staging/` directory.
    For each model:
    *   Use the `{{ source('staging_dlt', 'TableName') }}` macro to reference the raw table.
    *   Rename columns to a consistent snake_case format.
    *   Generate a surrogate key using `{{ dbt_utils.surrogate_key(['SourceIDColumn']) }}` for each primary key.
    *   Add descriptions and tests to `TP6/northwind_T/models/staging/schema.yml` for these new staging models.

    **Example for `stg_employees.sql`:**
    ```sql
    with source as (
        select * from {{ source('staging_dlt', 'Employees') }}
    ),
    renamed as (
        select
            {{ dbt_utils.surrogate_key(['EmployeeID']) }} as employee_pk,
            "EmployeeID" as employee_id,
            "LastName" as last_name,
            "FirstName" as first_name,
            "Title" as title,
            "TitleOfCourtesy" as title_of_courtesy,
            "BirthDate" as birth_date,
            "HireDate" as hire_date,
            "Address" as address,
            "City" as city,
            "Region" as region,
            "PostalCode" as postal_code,
            "Country" as country,
            "HomePhone" as home_phone,
            "Extension" as extension,
            "Notes" as notes,
            "ReportsTo" as reports_to,
            "PhotoPath" as photo_path
        from source
    )
    select * from renamed
    ```
    **Remember to add corresponding entries in `schema.yml` for `stg_employees`, `stg_products`, `stg_suppliers`, `stg_categories`, and `stg_order_details` with appropriate columns, descriptions, and tests (e.g., `unique`, `not_null`).**

4.  **Run dbt Models (Staging):**
    After creating your staging models, run them to materialize the views/tables in your Supabase database:
    ```bash
    dbt run --select stg_*
    ```
    Then, run the tests to ensure data quality:
    ```bash
    dbt test --select stg_*
    ```

### 2.3 Marts Layer (Dimension and Fact Tables)

The `marts` layer focuses on creating user-friendly dimension and fact tables for reporting and analysis.

1.  **Review Existing Marts Models:**
    Examine `TP6/northwind_T/models/marts/orders/schema.yml`. This file defines the expected structure and tests for `dim_customers` and `fct_orders`.
    You will also need to create `dim_customers.sql` and `fct_orders.sql`.

2.  **Create `dim_customers.sql` and `fct_orders.sql`:**
    Create the following files in `TP6/northwind_T/models/marts/orders/`:

    **`dim_customers.sql`:**
    ```sql
    with customers as (
        select * from {{ ref('stg_customers') }}
    ),
    orders as (
        select
            customer_id,
            min(order_date) as first_order_date
        from {{ ref('stg_orders') }}
        group by customer_id
    ),
    final as (
        select
            customers.customer_pk,
            customers.customer_id,
            customers.company_name,
            customers.contact_name,
            customers.contact_title,
            customers.address,
            customers.city,
            customers.region,
            customers.postal_code,
            customers.country,
            customers.phone,
            customers.fax,
            orders.first_order_date,
            customers.address || ', ' || customers.city ||
                coalesce(', ' || customers.region, '') || ', ' || customers.country as customer_full_address
        from customers
        left join orders on customers.customer_id = orders.customer_id
    )
    select * from final
    ```

    **`fct_orders.sql`:**
    ```sql
    with orders as (
        select * from {{ ref('stg_orders') }}
    ),
    order_details as (
        select
            order_id,
            sum(unit_price * quantity * (1 - discount)) as total_order_value
        from {{ ref('stg_order_details') }} -- Assuming you create stg_order_details
        group by order_id
    ),
    final as (
        select
            orders.order_pk,
            orders.order_id,
            orders.customer_id,
            orders.employee_id,
            orders.order_date,
            orders.required_date,
            orders.shipped_date,
            orders.ship_via,
            orders.freight,
            orders.ship_name,
            orders.ship_address,
            orders.ship_city,
            orders.ship_region,
            orders.ship_postal_code,
            orders.ship_country,
            order_details.total_order_value,
            case
                when orders.shipped_date is not null then 'Shipped'
                when orders.required_date < current_date then 'Pending'
                else 'Pending' -- You might refine this logic
            end as order_status_flag,
            julianday(orders.shipped_date) - julianday(orders.order_date) as shipping_delay_days
        from orders
        left join order_details on orders.order_id = order_details.order_id
    )
    select * from final
    ```

3.  **Create Dimension Models:**
    Create `dim_employees.sql`, `dim_products.sql`, `dim_suppliers.sql`, and `dim_categories.sql` in the `TP6/northwind_T/models/marts/` directory (or a subfolder like `dimensions/`).
    For each dimension model:
    *   Reference the corresponding `stg_` model (e.g., `{{ ref('stg_employees') }}`).
    *   Select all relevant columns.
    *   Add any derived attributes that are useful for analysis (e.g., `employee_full_name`, `product_category_name`).
    *   Add descriptions and tests to `TP6/northwind_T/models/marts/orders/schema.yml` (or a new `schema.yml` in a `dimensions` subfolder) for these new dimension models.

    **Example for `dim_employees.sql`:**
    ```sql
    with employees as (
        select * from {{ ref('stg_employees') }}
    ),
    final as (
        select
            employee_pk,
            employee_id,
            first_name,
            last_name,
            title,
            title_of_courtesy,
            birth_date,
            hire_date,
            address,
            city,
            region,
            postal_code,
            country,
            home_phone,
            extension,
            notes,
            reports_to,
            photo_path,
            first_name || ' ' || last_name as employee_full_name
        from employees
    )
    select * from final
    ```

4.  **Create `fct_order_details.sql` (The Central Fact Table):**
    This is the core of your data mart. It will combine information from `stg_order_details` with foreign keys to your dimension tables.
    Create `fct_order_details.sql` in `TP6/northwind_T/models/marts/orders/`.

    **`fct_order_details.sql`:**
    ```sql
    with order_details as (
        select * from {{ ref('stg_order_details') }}
    ),
    orders as (
        select * from {{ ref('stg_orders') }}
    ),
    products as (
        select * from {{ ref('stg_products') }}
    ),
    customers as (
        select * from {{ ref('stg_customers') }}
    ),
    employees as (
        select * from {{ ref('stg_employees') }}
    ),
    final as (
        select
            -- Primary Key
            {{ dbt_utils.surrogate_key(['order_details.order_id', 'order_details.product_id']) }} as order_detail_pk,

            -- Foreign Keys
            orders.order_pk,
            products.product_pk,
            customers.customer_pk,
            employees.employee_pk, -- Assuming you create dim_employees

            -- Measures / Facts
            order_details.unit_price,
            order_details.quantity,
            order_details.discount,
            (order_details.unit_price * order_details.quantity * (1 - order_details.discount)) as extended_price,

            -- Date Attributes (from orders)
            orders.order_date,
            orders.required_date,
            orders.shipped_date,

            -- Other relevant attributes from order_details
            order_details.order_id,
            order_details.product_id

        from order_details
        left join orders on order_details.order_id = orders.order_id
        left join products on order_details.product_id = products.product_id
        left join customers on orders.customer_id = customers.customer_id
        left join employees on orders.employee_id = employees.employee_id
    )
    select * from final
    ```
    **Remember to add corresponding entries in `schema.yml` for `dim_employees`, `dim_products`, `dim_suppliers`, `dim_categories`, and `fct_order_details` with appropriate columns, descriptions, and tests (e.g., `unique`, `not_null`, `relationships`).**

5.  **Run dbt Models (Marts):**
    After creating your marts models, run them:
    ```bash
    dbt run --select dim_* fct_*
    ```
    Then, run the tests:
    ```bash
    dbt test --select dim_*
    ```

## Conclusion

By completing this practical work, you will have successfully:
*   Ingested data from a SQLite database into Supabase PostgreSQL using `dlt`.
*   Built a `staging` layer in `dbt` to clean and standardize your raw data.
*   Constructed a `marts` layer with dimension tables (`dim_customers`, `dim_employees`, `dim_products`, `dim_suppliers`, `dim_categories`) and a central fact table (`fct_order_details`) for analytical purposes.

This structured data warehouse will serve as a solid foundation for further data analysis and visualization.