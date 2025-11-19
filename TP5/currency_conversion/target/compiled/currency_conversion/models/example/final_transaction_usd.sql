

with latest_rates as (
  -- نأخذ أحدث لقطة من جدول أسعار الصرف
  select
    as_of,
    -- سنعتمد على الحقول المقلوبة (usd_per_*) لتفادي مشاكل اتجاه السعر
    usd_per_eur,
    usd_per_gbp,
    usd_per_jpy
  from "currency_db"."raw"."currency_rates"
  order by as_of desc
  limit 1
),
tx as (
  -- أعمدة جدول raw.daily_transactions وفق ما ظهر لديك
  select
    transaction_id,
    (amount)::numeric as amount_original,
    currency,
    customer_id,
    transaction_date
  from "currency_db"."raw"."daily_transactions"
)

select
  t.transaction_id,
  t.customer_id,
  -- لو كانت صيغة التاريخ نصية، نحاول تحويلها لتوقيت؛ إن لم تنجح ستُصبح NULL بدون كسر الاستعلام
  nullif(t.transaction_date, '')::timestamp as transaction_ts,
  t.currency,
  t.amount_original,
  case
    when t.currency = 'USD' then t.amount_original
    when t.currency = 'EUR' then t.amount_original * r.usd_per_eur
    when t.currency = 'GBP' then t.amount_original * r.usd_per_gbp
    when t.currency = 'JPY' then t.amount_original * r.usd_per_jpy
    else null
  end as amount_usd,
  r.as_of as rate_as_of
from tx t
cross join latest_rates r