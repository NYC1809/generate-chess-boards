async function analysePosition(fen) {
  const response = await fetch(
    "https://chess-api.com/v1",
    {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        fen: fen,
        depth: 15,
        variants: 3
      })
    }
  );

  const result = await response.json();

  console.log("Evaluation:", result.eval);
  console.log("Best move:", result.san);
  console.log("Continuation:", result.continuationArr);
  console.log("Win chance:", result.winChance);

  return result;
}

// Analyse the starting position
analysePosition(
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
);
