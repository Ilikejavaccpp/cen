-- write some normal SEQUEL

CREATE TABLE IF NOT EXISTS widgets (
    id INTEGER PRIMARY KEY NOT NULL AUTOINCREMENT,
    name CHAR(255) NOT NULL,
    variable_name CHAR(255) NOT NULL,
    description TEXT, -- remember it's optional to be a bad dev and use emojis with no meaning
    position_x INTEGER NOT NULL,
    position_y INTEGER NOT NULL,
    parent CHAR(255), -- not the case for 1st element: the window
    parent_id INTEGER -- not the case for 1st element: the window
);

-- let's append some data predefined like window and the box.
INSERT INTO widgets (name, variable_name, description, position_x, position_y, parent, parent_id) VALUES
    ('GtkWindow', 'window', 'the main application window', 0, 0, NULL, NULL),
    ('GtkBox', 'box', 'a GtkBox that houses other widgets', 100, 100, 'window', 1);
