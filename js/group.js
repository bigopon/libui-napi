const {Group} = require('..');

/**
 * A container for a single widget that provide a caption and visually group it's children.
 */
class UiGroup {
	/**
	 * Create a new UiGroup object.
	 *
	 * @param {string} title - text to show in Group title bar.
	 * @return {UiGroup} new instance.
	 */
	constructor(title = '') {
		this.handle = Group.create(title);
	}

	/**
	 * Set or return the text to show in group caption.
	 *
	 * @return {string}
	 */
	get title() {
		return Group.getTitle(this.handle);
	}

	set title(value) {
		Group.setTitle(this.handle, String(value));
	}

	/**
	 * Set the control to show in this group content area.
	 * UiGroup instances can contain only one control. If you need
	 * more, you have to use [Containers](containers.md).
	 *
	 * @param {UiControl} control - the control to add as child.
	 * @param {boolean} stretchy - whever the control should fill all the available space.
	 */
	setChild(control, stretchy) {
		Group.setChild(this.handle, control.handle, Boolean(stretchy));
	}

	/**
	 * When true, an internal margin is added to the group.
	 *
	 * @return {boolean}
	 */
	get margined() {
		return Group.getMargined(this.handle);
	}

	set margined(value) {
		Group.setMargined(this.handle, Boolean(value));
	}
}

module.exports = {UiGroup};