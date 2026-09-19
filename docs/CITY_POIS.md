# Shops and POIs

The catalogue now extracts categorised POIs from OSM nodes, ways and relations. It remains a snapshot-backed gameplay/search index and must not be described as a live business directory.

## Runtime catalogue

Each POI preserves its source type, category, world/geographic position, raw OSM tags, raw `opening_hours`, wheelchair/access values, structured address fields and normalized search tokens. Missing accessibility remains `unknown`; opening hours are preserved rather than guessed.

Named POIs with the same category inside the configured spatial radius are deduplicated. Removed source IDs are retained in an alias map so saved references can resolve to the canonical POI.

Gameplay-service eligibility is limited to categories with defined mechanics; culture remains searchable without automatically implying a shop/economy interaction.
