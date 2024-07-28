#!/usr/bin/env bash

# aseprite --batch \
#     ./assets/sprites/d1.png \
#     ./assets/sprites/d2.png \
#     ./assets/sprites/d3.png \
#     ./assets/sprites/d4.png \
#     ./assets/sprites/button.png \
#     --sheet ./assets/atlas.png --data output.json

magick \
    \( ./assets/sprites/d1.png ./assets/sprites/d2.png ./assets/sprites/d3.png ./assets/sprites/d4.png ./assets/sprites/button.png +append \) \
    -append ./assets/atlas.png
    # \( musicals/follies/cover.png musicals/ghost-quartet/cover.png +append \) \
    # \( musicals/god-bless-you-mr-rosewater/cover.png musicals/godspell/cover.png musicals/hadestown/cover.png musicals/hamilton/cover.png musicals/howard-sings-ashman/cover.png musicals/in-trousers/cover.png musicals/into-the-woods/cover.png +append \) \
    # \( musicals/jesus-christ-superstar/cover.png musicals/kiss-me-kate/cover.png musicals/little-shop-of-horrors/cover.png musicals/march-of-the-falsettos/cover.png musicals/mass/cover.png musicals/merrily-we-roll-along/cover.png musicals/myths-and-hymns/cover.png +append \) \
    # \( musicals/natasha,-pierre-and-the-great-comet-of-1812/cover.png musicals/next-to-normal/cover.png musicals/octet/cover.png musicals/oklahoma/cover.png musicals/pacific-overtures/cover.png musicals/parade/cover.png musicals/pippin/cover.png +append \) \
    # \( musicals/preludes/cover.png musicals/ragtime/cover.png musicals/saturday-night/cover.png musicals/soft-power/cover.png musicals/sondheim-sings-I/cover.png musicals/south-pacific/cover.png musicals/spring-awakening-deutsch/cover.png +append \) \
    # \( musicals/sunday-in-the-park-with-george/cover.png musicals/sweeney-todd/cover.png musicals/the-25th-annual-putnam-county-spelling-bee/cover.png musicals/the-last-five-years/cover.png musicals/the-light-in-the-piazza/cover.png musicals/urintown/cover.png musicals/west-side-story/cover.png +append \) \

magick \
    \( ./assets/sprites/w_floor.png ./assets/sprites/w_bed.png ./assets/sprites/w_bed_end.png ./assets/sprites/w_window.png ./assets/sprites/w_door.png ./assets/sprites/w_table.png ./assets/sprites/w_blinds.png ./assets/sprites/w_boss.png ./assets/sprites/w_table_tl.png ./assets/sprites/w_table_tr.png ./assets/sprites/w_table_bl.png ./assets/sprites/w_table_br.png ./assets/sprites/w_chair.png ./assets/sprites/w_train.png +append \) \
    -append ./assets/world_atlas.png


magick \
    \( ./assets/sprites/player_pain.png ./assets/sprites/player_fatigue.png ./assets/sprites/player_sad.png ./assets/sprites/player_main.png ./assets/sprites/player_happy.png +append \) \
    -append ./assets/player_atlas.png
