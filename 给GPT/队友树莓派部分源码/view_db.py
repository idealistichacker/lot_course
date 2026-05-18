from langchain_community.vectorstores import Chroma
from langchain_community.embeddings import HuggingFaceEmbeddings

def show_db(category="greenhouse_1"):
    embed = HuggingFaceEmbeddings(model_name="all-MiniLM-L6-v2")

    db = Chroma(
        persist_directory=f"./db/{category}",
        embedding_function=embed
    )

    results = db.similarity_search("农业", k=20)

    print("\n==============================")
    print("📚 知识库内容:", category)
    print("==============================")

    for i, r in enumerate(results):
        print(f"\n[{i}] {r.page_content}")

if __name__ == "__main__":
    show_db("greenhouse")