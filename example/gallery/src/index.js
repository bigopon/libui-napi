import {start, ui} from '../../../js/fancy-api.js';
let win;

start(() => {
	win = (<window title='Control Gallery' width={800} height={600} margined={true}>
		<tab>
			<vbox label='Basic Controls'>
				{basicControlsPage}
			</vbox>
			<vbox label="Numbers and Lists">
				{numbersPage}
			</vbox>
			<vbox label='Data Choosers'>
				{dataChoosersPage}
			</vbox>
		</tab>
	</window>);
	return win;
});

const basicControlsPage = (
	<vbox padded>
		<hbox padded>
			<button text="Button"/>
			<checkbox text='Checkbox'/>
		</hbox>
		<label>
			This is a label. Right now,
			labels can only span one line.
		</label>
		<hseparator/>
		<group title='Entries' margined stretchy>
			<form padded stretchy>
				<entry label='Entry' />
				<password label='Password Entry'/>
				<search label='Search Entry'/>
				<textarea label='Wrapped Multiline Entry' stretchy wrapped></textarea>
				<textarea label="Multiline Entry" stretchy></textarea>
			</form>
		</group>
	</vbox>
);

const numbersPage = (
	<hbox padded>
		<group margined stretchy title='Numbers'>
			<spinbox min={0} max={100}/>
			<slider min={0} max={
		100}/>
			<progressbar value={42}/>
			<progressbar value={
		-1}/>
		</group>
		<group stretchy margined title='Lists'>
			<combobox items={
		['Combobox Item 1', 'Combobox Item 2', 'Combobox Item 3']}/>

			<editcombo items={[
				"Editable Item 1",
				"Editable Item 2",
				"Editable Item 3"]}/>

			<radio items={
		['Radio Item 1', 'Radio Item 2', 'Radio Item 3']}/>
		</group>

	</hbox>
);

const dataChoosersPage = (
	<hbox padded>
		<vbox padded>
			<date />
			<time />
			<datetime />
			<fontpicker />
			<colorpicker />
		</vbox>
		<vseparator />
		<vbox padded stretchy>
			<grid padded>
				<button
	left = {0} top = {0} xspan = {1} vspan = {1} hexpand = {false} halign =
		{ui.grid.align.fill} vexpand = {false} valign = {ui.grid.align.fill} text =
			'Open File'
	onClicked =
	{
		onOpenFileClicked
	} />
				<entry
					left={1}
					top={0}
					xspan={1}
					vspan={1}
					hexpand={true}
					halign={ui.grid.align.fill}
					vexpand={false}
					valign={ui.grid.align.fill}
					id="filename"
					readonly/ >

		< button
	left = {0} top = {1} xspan = {1} vspan = {1} hexpand = {false} halign =
		{ui.grid.align.fill} vexpand = {false} valign = {ui.grid.align.fill} text =
			'Save File'
	onClicked =
	{
		onSaveFileClicked
	} />
				<entry
					left={1}
					top={1}
					xspan={1}
					vspan={1}
					hexpand={true}
					halign={ui.grid.align.fill}
					vexpand={false}
					valign={ui.grid.align.fill}
					id="saveFilename"
					readonly/ >

		< grid
	padded
	left = {0} top = {2} xspan = {2} vspan = {1} hexpand = {false} halign = {
		ui.grid.align.center} vexpand = {false} valign = {ui.grid.align.start} > < button
	left = {0} top = {0} xspan = {1} vspan = {1} hexpand = {false} halign =
		{ui.grid.align.fill} vexpand = {false} valign = {ui.grid.align.fill} text =
			'Message Box'
						onClicked={
		onMsgboxClicked}
						/>

					<button
						left={1}
						top={0}
						xspan={1}
						vspan={1}
						hexpand={false}
						halign={ui.grid.align.fill}
						vexpand={false}
						valign={ui.grid.align.fill}
						text="Error Box"
						onClicked={onErrorboxClicked}
						/>
				</grid>
			</grid>
		</vbox>
	</hbox>
);

						function onErrorboxClicked() {
							ui.msgBoxError(win, 'Error', 'This is a message');
						}

						function onMsgboxClicked() {
							ui.msgBox(win, 'Error', 'This is an error');
						}

						function onOpenFileClicked() {
							win.filename.text = ui.openFile(win);
						}

						function onSaveFileClicked() {
							win.saveFilename.text = ui.saveFile(win);
						}