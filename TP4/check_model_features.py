import pickle

with open("models/lr_pipeline.pkl", "rb") as f:
    model = pickle.load(f)

if hasattr(model, "feature_names_in_"):
    print("Feature names from pipeline:", list(model.feature_names_in_))
else:
    print("Model does not store feature names.")
