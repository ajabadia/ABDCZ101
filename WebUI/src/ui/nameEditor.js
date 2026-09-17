// Name Editor overlay (native NameEditorOverlay parity) — extracted module.
// Styled dialog: RENAME PRESET title in cyan, centered text editor, SAVE/CANCEL
// buttons, Enter saves / Escape cancels (native onReturnKey/onEscapeKey), shown
// over a semi-transparent backdrop. One instance reused for every rename/move
// entry point (bank context menu, File -> Rename Current Patch, Store to New
// Slot) — replacing the browser prompt()/focus-select flow.

export function createNameEditor() {
// ─── Name Editor overlay (native NameEditorOverlay parity) ───
// Styled dialog: RENAME PRESET title in cyan, centered text editor, SAVE/CANCEL
// buttons, Enter saves / Escape cancels (native onReturnKey/onEscapeKey), shown
// over a semi-transparent backdrop. One instance reused for every rename/move
// entry point (bank context menu, File -> Rename Current Patch, Store to New
// Slot) — replacing the browser prompt()/focus-select flow.
const nameEditorModal = document.getElementById('name-editor-modal');
const nameEditorTitle = document.getElementById('name-editor-title');
const nameEditorInput = document.getElementById('name-editor-input');
const btnNameEditorSave = document.getElementById('btn-name-editor-save');
const btnNameEditorCancel = document.getElementById('btn-name-editor-cancel');

let nameEditorOnSave = null;

const openNameEditor = ({ title = 'RENAME PRESET', value = '', onSave = null }) => {
  if (!nameEditorModal) return;
  nameEditorTitle.innerText = title;
  nameEditorInput.value = value;
  nameEditorOnSave = onSave;
  nameEditorModal.hidden = false;
  nameEditorInput.focus();
  nameEditorInput.select(); // native: grabKeyboardFocus() + pre-selected text
};

const closeNameEditor = () => {
  if (!nameEditorModal) return;
  nameEditorModal.hidden = true;
  nameEditorOnSave = null;
};

if (btnNameEditorSave) {
  btnNameEditorSave.addEventListener('click', () => {
    const cb = nameEditorOnSave;
    closeNameEditor();
    if (cb) cb(nameEditorInput.value);
  });
}

if (btnNameEditorCancel) {
  btnNameEditorCancel.addEventListener('click', closeNameEditor);
}

if (nameEditorInput) {
  nameEditorInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') { e.preventDefault(); btnNameEditorSave && btnNameEditorSave.click(); }
    else if (e.key === 'Escape') { e.preventDefault(); closeNameEditor(); }
  });
}

// Clicking the backdrop behaves like CANCEL (native: the overlay hides when the
// dialog is dismissed; there is no other way out).
if (nameEditorModal) {
  nameEditorModal.addEventListener('click', (e) => {
    if (e.target === nameEditorModal) closeNameEditor();
  });
}

  return { openNameEditor, closeNameEditor };
}
