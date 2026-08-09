#include "drawing_program/drawing_program_visual_input_indexed_asset.h"

#include <stdio.h>

#include "drawing_program/drawing_program_indexed_cells.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"

static void indexed_asset_set_status(DrawingProgramAppContext *ctx, const char *message) {
    if (ctx && message) {
        (void)snprintf(ctx->session.file_action_status_message,
                       sizeof(ctx->session.file_action_status_message),
                       "%s",
                       message);
    }
}

static int indexed_asset_rect_occupied(const DrawingProgramIndexedCellTable *table,
                                       uint32_t selected,
                                       uint32_t x,
                                       uint32_t y) {
    if (!table) return 0;
    for (uint32_t i = 0u; i < table->count; ++i) {
        if (i != selected && table->cells[i].x == x && table->cells[i].y == y) return 1;
    }
    return 0;
}

int drawing_program_visual_input_handle_indexed_asset(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    const uint32_t action_count = 4u;
    DrawingProgramIndexedCellTable *table;
    const DrawingProgramIndexedTilesetProfile *profile;
    VisualPaneLayoutMetrics metrics;
    SDL_Rect queue;
    SDL_Rect add_button;
    SDL_Rect rename_button;
    SDL_Rect move_button;
    SDL_Rect workspace_button;
    uint32_t row_count;
    int scroll_y;
    if (!ctx || !ui || !hooks ||
        ctx->texture_project.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1) {
        return 0;
    }
    table = &ctx->texture_project.indexed_cells;
    profile = &ctx->texture_project.indexed_profile;
    metrics = make_pane_layout_metrics(ctx);
    queue = right_asset_target_queue_rect(
        rect, metrics, VISUAL_RIGHT_PANEL_ASSET_TAB_FOOTER_LINE_COUNT, action_count);
    add_button = right_file_route_action_button_rect(
        rect, metrics, VISUAL_RIGHT_PANEL_ASSET_TAB_FOOTER_LINE_COUNT, 0u, action_count);
    rename_button = right_file_route_action_button_rect(
        rect, metrics, VISUAL_RIGHT_PANEL_ASSET_TAB_FOOTER_LINE_COUNT, 1u, action_count);
    move_button = right_file_route_action_button_rect(
        rect, metrics, VISUAL_RIGHT_PANEL_ASSET_TAB_FOOTER_LINE_COUNT, 2u, action_count);
    workspace_button = right_file_route_action_button_rect(
        rect, metrics, VISUAL_RIGHT_PANEL_ASSET_TAB_FOOTER_LINE_COUNT, 3u, action_count);
    row_count = table->count ? table->count : 1u;
    scroll_y = right_file_target_queue_clamp_scroll(
        queue, metrics, row_count, ui->right_file_target_queue_scroll_y);
    if (hooks->point_in_rect(queue, x, y)) {
        for (uint32_t i = 0u; i < table->count; ++i) {
            SDL_Rect row = right_file_target_queue_row_rect(queue, metrics, i, scroll_y);
            if (hooks->point_in_rect(row, x, y)) {
                ctx->ui.indexed_selected_cell = (uint8_t)i;
                ctx->ui.indexed_workspace_mode = (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_CELL_BOARD;
                indexed_asset_set_status(ctx, "CELL SELECTED IN BOARD");
                return 1;
            }
        }
        return 1;
    }
    if (hooks->point_in_rect(add_button, x, y)) {
        uint32_t columns = profile->atlas_width / profile->logical_cell_width;
        uint32_t rows = profile->atlas_height / profile->logical_cell_height;
        for (uint32_t slot = 0u; slot < columns * rows; ++slot) {
            uint32_t cell_x = (slot % columns) * profile->logical_cell_width;
            uint32_t cell_y = (slot / columns) * profile->logical_cell_height;
            char id[DRAWING_PROGRAM_INDEXED_CELL_ID_CAPACITY];
            if (indexed_asset_rect_occupied(table, UINT32_MAX, cell_x, cell_y)) continue;
            (void)snprintf(id, sizeof(id), "cell.%u", (unsigned)(table->count + 1u));
            if (drawing_program_indexed_cell_create(
                    table, &ctx->texture_project.indexed_cell_history, id, cell_x, cell_y,
                    profile->logical_cell_width, profile->logical_cell_height).code == CORE_OK) {
                ctx->ui.indexed_selected_cell = (uint8_t)(table->count - 1u);
                indexed_asset_set_status(ctx, "CELL ADDED");
                return 1;
            }
        }
        indexed_asset_set_status(ctx, "NO FREE CELL RECT");
        return 1;
    }
    if (hooks->point_in_rect(rename_button, x, y) && table->count > 0u) {
        char id[DRAWING_PROGRAM_INDEXED_CELL_ID_CAPACITY];
        uint32_t selected = ctx->ui.indexed_selected_cell < table->count
            ? ctx->ui.indexed_selected_cell : 0u;
        uint32_t suffix = 1u;
        do {
            (void)snprintf(id, sizeof(id), "cell.custom.%u", (unsigned)suffix++);
        } while (drawing_program_indexed_cell_table_find(table, id) && suffix < 10000u);
        if (drawing_program_indexed_cell_rename(
                table, &ctx->texture_project.indexed_cell_history, selected, id).code == CORE_OK) {
            indexed_asset_set_status(ctx, "CELL RENAMED");
        } else {
            indexed_asset_set_status(ctx, "CELL RENAME FAILED");
        }
        return 1;
    }
    if (hooks->point_in_rect(move_button, x, y) && table->count > 0u) {
        uint32_t selected = ctx->ui.indexed_selected_cell < table->count
            ? ctx->ui.indexed_selected_cell : 0u;
        uint32_t columns = profile->atlas_width / profile->logical_cell_width;
        uint32_t rows = profile->atlas_height / profile->logical_cell_height;
        for (uint32_t slot = 0u; slot < columns * rows; ++slot) {
            uint32_t cell_x = (slot % columns) * profile->logical_cell_width;
            uint32_t cell_y = (slot / columns) * profile->logical_cell_height;
            if (!indexed_asset_rect_occupied(table, selected, cell_x, cell_y) &&
                (table->cells[selected].x != cell_x || table->cells[selected].y != cell_y)) {
                (void)drawing_program_indexed_cell_set_rect(
                    table, &ctx->texture_project.indexed_cell_history, selected, cell_x, cell_y,
                    profile->logical_cell_width, profile->logical_cell_height);
                indexed_asset_set_status(ctx, "CELL RECT MOVED");
                return 1;
            }
        }
        indexed_asset_set_status(ctx, "NO FREE CELL RECT");
        return 1;
    }
    if (hooks->point_in_rect(workspace_button, x, y)) {
        ctx->ui.indexed_workspace_mode =
            ctx->ui.indexed_workspace_mode == (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_CELL_BOARD
                ? (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_ATLAS
                : (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_CELL_BOARD;
        indexed_asset_set_status(ctx,
            ctx->ui.indexed_workspace_mode == (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_CELL_BOARD
                ? "CELL BOARD SHOWN" : "ATLAS SHOWN");
        return 1;
    }
    return 0;
}
